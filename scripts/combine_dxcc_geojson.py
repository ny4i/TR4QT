#!/usr/bin/env python3
"""
combine_dxcc_geojson.py - Generate combined world DXCC GeoJSON file for TR4QT

This script reads the DXCC-to-GeoJSON mapping file and combines individual
country/territory GeoJSON files into a single file with DXCC properties.

Usage:
    python3 scripts/combine_dxcc_geojson.py

Requirements:
    - Python 3.6+
    - world-geojson repository at ~/projects/world-geojson/

Output:
    - resources/world_dxcc.geojson
"""

import json
import os
import sys
from pathlib import Path

# Configuration
WORLD_GEOJSON_PATH = Path.home() / "projects" / "world-geojson"
MAPPING_FILE = Path(__file__).parent.parent / "resources" / "dxcc_geojson_mapping.json"
OUTPUT_FILE = Path(__file__).parent.parent / "resources" / "world_dxcc.geojson"

# Point marker size for entities without polygons (tiny islands, etc.)
POINT_MARKER_RADIUS = 0.5  # degrees


def load_mapping():
    """Load the DXCC-to-GeoJSON mapping file."""
    with open(MAPPING_FILE, 'r') as f:
        data = json.load(f)
    return data['mappings']


def load_geojson(source_path):
    """Load a GeoJSON file from the world-geojson repository."""
    full_path = WORLD_GEOJSON_PATH / source_path
    if not full_path.exists():
        return None

    with open(full_path, 'r') as f:
        return json.load(f)


def create_point_feature(dxcc, name, lon, lat):
    """Create a point feature for entities without polygon data."""
    return {
        "type": "Feature",
        "properties": {
            "dxcc": dxcc,
            "name": name
        },
        "geometry": {
            "type": "Point",
            "coordinates": [lon, lat]
        }
    }


def create_circle_polygon(lon, lat, radius=POINT_MARKER_RADIUS, segments=16):
    """
    Create a circular polygon for point markers.
    This makes them visible on the map since points are hard to see.
    """
    import math
    coords = []
    for i in range(segments + 1):
        angle = 2 * math.pi * i / segments
        x = lon + radius * math.cos(angle)
        y = lat + radius * math.sin(angle)
        coords.append([x, y])
    return coords


def create_circle_feature(dxcc, name, lon, lat):
    """Create a small circle polygon for entities without polygon data."""
    return {
        "type": "Feature",
        "properties": {
            "dxcc": dxcc,
            "name": name
        },
        "geometry": {
            "type": "Polygon",
            "coordinates": [create_circle_polygon(lon, lat)]
        }
    }


def is_artifact_polygon(geometry):
    """
    Check if a geometry is a clipping artifact (narrow strip spanning the world).
    Real landmasses may have horizontal edges but span a wide latitude range.
    Artifacts are thin strips (< 5° latitude) spanning most of the world.
    """
    def get_lat_range(coords, geom_type):
        """Get the total latitude range of a geometry."""
        all_lats = []
        if geom_type == "Polygon":
            for ring in coords:
                for pt in ring:
                    all_lats.append(pt[1])
        elif geom_type == "MultiPolygon":
            for poly in coords:
                for ring in poly:
                    for pt in ring:
                        all_lats.append(pt[1])
        if all_lats:
            return max(all_lats) - min(all_lats)
        return 0

    def has_world_spanning_edge(coords, geom_type):
        """Check if geometry has a horizontal edge spanning > 300° longitude."""
        def check_ring(ring):
            for i in range(len(ring) - 1):
                lat1, lat2 = ring[i][1], ring[i + 1][1]
                lon1, lon2 = ring[i][0], ring[i + 1][0]
                lon_span = abs(lon2 - lon1)
                if abs(lat1 - lat2) < 2.0 and lon_span > 300:
                    return True
            return False

        if geom_type == "Polygon":
            for ring in coords:
                if check_ring(ring):
                    return True
        elif geom_type == "MultiPolygon":
            for poly in coords:
                for ring in poly:
                    if check_ring(ring):
                        return True
        return False

    geom_type = geometry.get("type")
    coords = geometry.get("coordinates", [])

    # Only filter if: has world-spanning edge AND narrow latitude range (< 5°)
    # This keeps legitimate countries like Russia while filtering thin artifact strips
    if has_world_spanning_edge(coords, geom_type):
        lat_range = get_lat_range(coords, geom_type)
        if lat_range < 5.0:  # Artifact: thin strip spanning the world
            return True
    return False


def add_dxcc_property(feature, dxcc, name):
    """Add DXCC prefix property to a feature."""
    if feature.get("properties") is None:
        feature["properties"] = {}
    feature["properties"]["dxcc"] = dxcc
    feature["properties"]["name"] = name
    return feature


def simplify_coordinates(coords, tolerance=0.01):
    """
    Simple coordinate simplification using Douglas-Peucker-like approach.
    Removes points that are within tolerance of their neighbors.
    """
    if len(coords) <= 2:
        return coords

    simplified = [coords[0]]
    for i in range(1, len(coords) - 1):
        prev = simplified[-1]
        curr = coords[i]
        # Keep point if it's far enough from previous
        dist = ((curr[0] - prev[0])**2 + (curr[1] - prev[1])**2)**0.5
        if dist >= tolerance:
            simplified.append(curr)

    simplified.append(coords[-1])
    return simplified


def simplify_geometry(geometry, tolerance=0.01):
    """Simplify geometry to reduce file size."""
    geom_type = geometry.get("type")

    if geom_type == "Polygon":
        simplified_rings = []
        for ring in geometry["coordinates"]:
            simplified = simplify_coordinates(ring, tolerance)
            if len(simplified) >= 4:  # Minimum for a valid polygon ring
                simplified_rings.append(simplified)
        if simplified_rings:
            geometry["coordinates"] = simplified_rings

    elif geom_type == "MultiPolygon":
        simplified_polys = []
        for poly in geometry["coordinates"]:
            simplified_rings = []
            for ring in poly:
                simplified = simplify_coordinates(ring, tolerance)
                if len(simplified) >= 4:
                    simplified_rings.append(simplified)
            if simplified_rings:
                simplified_polys.append(simplified_rings)
        if simplified_polys:
            geometry["coordinates"] = simplified_polys

    return geometry


def round_coordinates(coords, precision=3):
    """Round coordinates to reduce file size."""
    if isinstance(coords[0], list):
        return [round_coordinates(c, precision) for c in coords]
    return [round(coords[0], precision), round(coords[1], precision)]


def process_mapping(mapping):
    """Process a single DXCC mapping entry and return features."""
    dxcc = mapping["dxcc"]
    name = mapping["name"]
    source = mapping.get("source")
    point = mapping.get("point")

    features = []

    if source:
        # Load from GeoJSON file
        geojson = load_geojson(source)
        if geojson is None:
            print(f"  WARNING: File not found: {source} for {dxcc} ({name})")
            # Fall back to point if available
            if point:
                features.append(create_circle_feature(dxcc, name, point[0], point[1]))
            return features

        # Process features from the file
        skipped_artifacts = 0
        if geojson.get("type") == "FeatureCollection":
            for feature in geojson.get("features", []):
                # Skip artifact polygons (thin strips spanning the world)
                if "geometry" in feature and is_artifact_polygon(feature["geometry"]):
                    skipped_artifacts += 1
                    continue
                feature = add_dxcc_property(feature.copy(), dxcc, name)
                # Simplify and round coordinates
                if "geometry" in feature:
                    feature["geometry"] = simplify_geometry(feature["geometry"])
                    if "coordinates" in feature["geometry"]:
                        feature["geometry"]["coordinates"] = round_coordinates(
                            feature["geometry"]["coordinates"]
                        )
                features.append(feature)
        elif geojson.get("type") == "Feature":
            # Skip artifact polygons (thin strips spanning the world)
            if "geometry" in geojson and is_artifact_polygon(geojson["geometry"]):
                skipped_artifacts += 1
            else:
                feature = add_dxcc_property(geojson.copy(), dxcc, name)
                if "geometry" in feature:
                    feature["geometry"] = simplify_geometry(feature["geometry"])
                    if "coordinates" in feature["geometry"]:
                        feature["geometry"]["coordinates"] = round_coordinates(
                            feature["geometry"]["coordinates"]
                        )
                features.append(feature)

        if skipped_artifacts > 0:
            print(f"    (skipped {skipped_artifacts} artifact polygons)")

    elif point:
        # Create a small circle for point-only entities
        features.append(create_circle_feature(dxcc, name, point[0], point[1]))
    else:
        print(f"  WARNING: No source or point for {dxcc} ({name})")

    return features


def main():
    print("TR4QT DXCC GeoJSON Combiner")
    print("=" * 40)

    # Check for world-geojson repository
    if not WORLD_GEOJSON_PATH.exists():
        print(f"ERROR: world-geojson repository not found at {WORLD_GEOJSON_PATH}")
        print("Please clone it: git clone https://github.com/johan/world.geo.json ~/projects/world-geojson")
        sys.exit(1)

    # Load mapping
    print(f"Loading mapping from {MAPPING_FILE}")
    mappings = load_mapping()
    print(f"Found {len(mappings)} DXCC entities in mapping")

    # Process all mappings
    print("\nProcessing DXCC entities...")
    all_features = []
    successful = 0
    failed = 0

    for mapping in mappings:
        features = process_mapping(mapping)
        if features:
            all_features.extend(features)
            successful += 1
            print(f"  OK: {mapping['dxcc']} - {mapping['name']} ({len(features)} features)")
        else:
            failed += 1

    # Create output FeatureCollection
    output = {
        "type": "FeatureCollection",
        "properties": {
            "generator": "TR4QT combine_dxcc_geojson.py",
            "totalEntities": successful,
            "version": "1.0.0"
        },
        "features": all_features
    }

    # Write output
    print(f"\nWriting output to {OUTPUT_FILE}")
    with open(OUTPUT_FILE, 'w') as f:
        json.dump(output, f, separators=(',', ':'))  # Minified output

    # Report file size
    size_bytes = OUTPUT_FILE.stat().st_size
    if size_bytes > 1024 * 1024:
        size_str = f"{size_bytes / (1024*1024):.1f} MB"
    else:
        size_str = f"{size_bytes / 1024:.1f} KB"

    print(f"\nSummary:")
    print(f"  Successful: {successful} entities")
    print(f"  Failed: {failed} entities")
    print(f"  Total features: {len(all_features)}")
    print(f"  Output file size: {size_str}")
    print("\nDone!")


if __name__ == "__main__":
    main()
