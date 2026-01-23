#!/usr/bin/env python3
"""
Combine individual US state GeoJSON files into a single FeatureCollection
for the TR4QT states map viewer.
"""

import json
import os
import sys

# Map state names to abbreviations
STATE_ABBREVS = {
    'alabama': 'AL', 'alaska': 'AK', 'arizona': 'AZ', 'arkansas': 'AR',
    'california': 'CA', 'colorado': 'CO', 'connecticut': 'CT', 'delaware': 'DE',
    'district_of_columbia': 'DC', 'florida': 'FL', 'georgia': 'GA',
    'hawaii': 'HI', 'idaho': 'ID', 'illinois': 'IL', 'indiana': 'IN',
    'iowa': 'IA', 'kansas': 'KS', 'kentucky': 'KY', 'louisiana': 'LA',
    'maine': 'ME', 'maryland': 'MD', 'massachusetts': 'MA', 'michigan': 'MI',
    'minnesota': 'MN', 'mississippi': 'MS', 'missouri': 'MO', 'montana': 'MT',
    'nebraska': 'NE', 'nevada': 'NV', 'new_hampshire': 'NH', 'new_jersey': 'NJ',
    'new_mexico': 'NM', 'new_york': 'NY', 'north_carolina': 'NC', 'north_dakota': 'ND',
    'ohio': 'OH', 'oklahoma': 'OK', 'oregon': 'OR', 'pennsylvania': 'PA',
    'rhode_island': 'RI', 'south_carolina': 'SC', 'south_dakota': 'SD',
    'tennessee': 'TN', 'texas': 'TX', 'utah': 'UT', 'vermont': 'VT',
    'virginia': 'VA', 'washington': 'WA', 'west_virginia': 'WV',
    'wisconsin': 'WI', 'wyoming': 'WY'
}

def combine_states(input_dir, output_file):
    """Combine individual state GeoJSON files into one FeatureCollection"""

    geojson = {
        "type": "FeatureCollection",
        "features": []
    }

    for filename in sorted(os.listdir(input_dir)):
        if not filename.endswith('.json'):
            continue

        state_name = filename.replace('.json', '')
        state_abbrev = STATE_ABBREVS.get(state_name)

        if not state_abbrev:
            print(f"Warning: No abbreviation for {state_name}, skipping")
            continue

        filepath = os.path.join(input_dir, filename)

        try:
            with open(filepath, 'r') as f:
                state_data = json.load(f)

            # The file might be a Feature or FeatureCollection
            if state_data.get('type') == 'FeatureCollection':
                # Multiple features - take all geometries
                for feature in state_data.get('features', []):
                    new_feature = {
                        "type": "Feature",
                        "properties": {"state": state_abbrev},
                        "geometry": feature.get('geometry')
                    }
                    geojson["features"].append(new_feature)
            elif state_data.get('type') == 'Feature':
                # Single feature
                new_feature = {
                    "type": "Feature",
                    "properties": {"state": state_abbrev},
                    "geometry": state_data.get('geometry')
                }
                geojson["features"].append(new_feature)
            elif state_data.get('type') in ['Polygon', 'MultiPolygon']:
                # Just geometry
                new_feature = {
                    "type": "Feature",
                    "properties": {"state": state_abbrev},
                    "geometry": state_data
                }
                geojson["features"].append(new_feature)
            else:
                print(f"Warning: Unknown format for {state_name}")
                continue

            print(f"Added {state_abbrev} ({state_name})")

        except Exception as e:
            print(f"Error processing {state_name}: {e}")
            continue

    # Write output
    with open(output_file, 'w') as f:
        json.dump(geojson, f, separators=(',', ':'))

    size_mb = os.path.getsize(output_file) / (1024 * 1024)
    print(f"\nWrote {len(geojson['features'])} features to {output_file}")
    print(f"File size: {size_mb:.2f} MB")

    return True

if __name__ == "__main__":
    input_dir = os.path.expanduser("~/projects/world-geojson/states/usa")
    output_file = "/Users/toms/projects/TR4QT/resources/us_states.geojson"

    success = combine_states(input_dir, output_file)
    sys.exit(0 if success else 1)
