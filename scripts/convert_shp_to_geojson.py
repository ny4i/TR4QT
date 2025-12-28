#!/usr/bin/env python3
"""
Convert ARRL Section shapefiles to a single GeoJSON file for web display
"""

import json
import glob
import os
import sys

try:
    import shapefile
except ImportError:
    print("ERROR: pyshp library not installed")
    print("Install with: pip3 install pyshp")
    sys.exit(1)

def convert_shapefiles_to_geojson(shapes_dir, output_file):
    """Convert all .shp files in directory to a single GeoJSON FeatureCollection"""

    # Find all .shp files
    shp_files = glob.glob(os.path.join(shapes_dir, "*.shp"))

    if not shp_files:
        print(f"No .shp files found in {shapes_dir}")
        return False

    print(f"Found {len(shp_files)} shapefile(s)")

    # Create GeoJSON FeatureCollection
    geojson = {
        "type": "FeatureCollection",
        "features": []
    }

    for shp_file in sorted(shp_files):
        section_code = os.path.splitext(os.path.basename(shp_file))[0]

        try:
            # Read shapefile
            sf = shapefile.Reader(shp_file)

            # Convert each shape to a GeoJSON feature
            for shape_record in sf.shapeRecords():
                shape = shape_record.shape

                # Convert coordinates
                if shape.shapeType == 5:  # Polygon
                    # Handle polygon with potential holes
                    parts = list(shape.parts) + [len(shape.points)]
                    rings = []

                    for i in range(len(parts) - 1):
                        start = parts[i]
                        end = parts[i + 1]
                        ring = [[pt[0], pt[1]] for pt in shape.points[start:end]]
                        rings.append(ring)

                    geometry = {
                        "type": "Polygon",
                        "coordinates": rings
                    }

                elif shape.shapeType == 15:  # PolygonZ (3D polygon)
                    # Same as polygon, but ignore Z coordinate
                    parts = list(shape.parts) + [len(shape.points)]
                    rings = []

                    for i in range(len(parts) - 1):
                        start = parts[i]
                        end = parts[i + 1]
                        ring = [[pt[0], pt[1]] for pt in shape.points[start:end]]
                        rings.append(ring)

                    geometry = {
                        "type": "Polygon",
                        "coordinates": rings
                    }

                else:
                    print(f"Warning: Unsupported shape type {shape.shapeType} in {section_code}")
                    continue

                # Create GeoJSON feature
                feature = {
                    "type": "Feature",
                    "properties": {
                        "section": section_code
                    },
                    "geometry": geometry
                }

                geojson["features"].append(feature)

            print(f"Converted {section_code}: {len(sf.shapes())} polygon(s)")

        except Exception as e:
            print(f"Error processing {section_code}: {e}")
            continue

    # Write GeoJSON file
    try:
        with open(output_file, 'w') as f:
            json.dump(geojson, f, separators=(',', ':'))  # Compact format
        print(f"\nSuccessfully wrote {len(geojson['features'])} features to {output_file}")

        # Print file size
        size_mb = os.path.getsize(output_file) / (1024 * 1024)
        print(f"File size: {size_mb:.2f} MB")

        return True

    except Exception as e:
        print(f"Error writing output file: {e}")
        return False

if __name__ == "__main__":
    shapes_dir = "/Users/toms/projects/n1mm_view/shapes"
    output_file = "/Users/toms/projects/TR4QT/resources/arrl_sections.geojson"

    # Create resources directory if it doesn't exist
    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    success = convert_shapefiles_to_geojson(shapes_dir, output_file)
    sys.exit(0 if success else 1)
