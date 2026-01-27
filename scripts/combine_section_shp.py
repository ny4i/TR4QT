#!/usr/bin/env python3
"""
Combine individual ARRL section shapefiles into per-state shapefiles
for multi-section states.

Reads individual section .shp files and produces one combined .shp file
per state, with a 'SECTION' attribute identifying each polygon.

Output: .shp, .shx, .dbf, .prj files per state.
"""

import os
import shutil
import sys

try:
    import shapefile
except ImportError:
    print("ERROR: pyshp library not installed")
    print("Install with: pip3 install pyshp")
    sys.exit(1)

# Multi-section states: state abbreviation -> list of section codes
MULTI_SECTION_STATES = {
    "FL": ["NFL", "WCF", "SFL"],
    "CA": ["EB", "LAX", "ORG", "SB", "SCV", "SDG", "SF", "SJV", "SV", "PAC"],
    "TX": ["NTX", "STX", "WTX"],
    "NY": ["NLI", "NNY", "WNY", "ENY"],
    "NJ": ["NNJ", "SNJ"],
    "MA": ["EMA", "WMA"],
    "PA": ["EPA", "WPA"],
    "WA": ["EWA", "WWA"],
}


def combine_sections(shapes_dir, output_dir):
    """Combine section shapefiles into per-state shapefiles."""

    os.makedirs(output_dir, exist_ok=True)

    for state, sections in sorted(MULTI_SECTION_STATES.items()):
        print(f"\n--- {state} ({len(sections)} sections: {', '.join(sections)}) ---")

        # Create writer with Polygon type (shapeType=5)
        w = shapefile.Writer(os.path.join(output_dir, state))
        w.shapeType = 5  # Polygon

        # Define fields: section code + state
        w.field("SECTION", "C", size=10)
        w.field("STATE", "C", size=2)

        prj_source = None
        total_shapes = 0

        for section_code in sections:
            shp_path = os.path.join(shapes_dir, section_code)

            if not os.path.exists(shp_path + ".shp"):
                print(f"  WARNING: {section_code}.shp not found, skipping")
                continue

            try:
                sf = shapefile.Reader(shp_path)
                shapes = sf.shapes()

                for shape in shapes:
                    # Copy the polygon geometry
                    w.shape(shape)
                    # Write the record with section and state identification
                    w.record(SECTION=section_code, STATE=state)
                    total_shapes += 1

                print(f"  {section_code}: {len(shapes)} polygon(s) added")

                # Save first valid .prj file path for copying
                prj_file = shp_path + ".prj"
                if prj_source is None and os.path.exists(prj_file):
                    prj_source = prj_file

            except Exception as e:
                print(f"  ERROR processing {section_code}: {e}")
                continue

        # Close the writer (writes .shp, .shx, .dbf)
        w.close()

        # Copy .prj from one of the sections (CRS definition)
        if prj_source:
            dest_prj = os.path.join(output_dir, state + ".prj")
            shutil.copy2(prj_source, dest_prj)
            print(f"  .prj copied from {os.path.basename(prj_source)}")

        print(f"  => {state}: {total_shapes} total shapes written")

        # Verify output
        try:
            verify = shapefile.Reader(os.path.join(output_dir, state))
            v_shapes = verify.shapes()
            v_records = verify.records()
            print(f"  Verify: {len(v_shapes)} shapes, {len(v_records)} records")
            for rec in v_records:
                print(f"    Section={rec['SECTION']}, State={rec['STATE']}")
        except Exception as e:
            print(f"  Verify FAILED: {e}")


def main():
    shapes_dir = "/Users/toms/projects/n1mm_view/shapes"
    output_dir = "/Users/toms/projects/TR4QT/resources/shp/multi_section_states"

    if not os.path.isdir(shapes_dir):
        print(f"ERROR: Source directory not found: {shapes_dir}")
        sys.exit(1)

    print(f"Source: {shapes_dir}")
    print(f"Output: {output_dir}")

    combine_sections(shapes_dir, output_dir)

    # Summary
    print("\n=== Output Files ===")
    for ext in [".shp", ".shx", ".dbf", ".prj"]:
        files = [f for f in os.listdir(output_dir) if f.endswith(ext)]
        print(f"  {ext}: {', '.join(sorted(files))}")

    print(f"\nAll files in: {output_dir}")


if __name__ == "__main__":
    main()
