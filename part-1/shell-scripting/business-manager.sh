#!/usr/bin/env bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
file="$SCRIPT_DIR/Businesses.csv"

echo "Enter the business CSV file path, or press Enter to use the default:"
echo "Default: $file"
read -r custom_file

if [ -n "$custom_file" ]; then
    file="$custom_file"
fi

if [ ! -f "$file" ]; then
    echo "File not found: $file"
    exit 1
fi

while true; do
    echo
    echo "Choose an option:"
    echo "[1] Select business CSV file"
    echo "[2] Display business by ID"
    echo "[3] Update business field"
    echo "[4] View file"
    echo "[5] Save file copy"
    echo "[6] Exit"
    echo
    read -r choice

    case "$choice" in
        1)
            echo "Enter the business CSV file path:"
            read -r custom_file

            if [ -n "$custom_file" ] && [ -f "$custom_file" ]; then
                file="$custom_file"
                echo "Selected file: $file"
            else
                echo "File not found."
            fi
            ;;

        2)
            echo "Enter business ID:"
            read -r business_code

            # Match the requested ID against the first CSV field.
            awk -F',' -v code="$business_code" '
                NR == 1 { header = $0; next }
                $1 == code {
                    print header
                    print $0
                    found = 1
                }
                END {
                    if (!found) {
                        print "No business found with this ID." > "/dev/stderr"
                    }
                }
            ' "$file"
            ;;

        3)
            echo "Enter business ID:"
            read -r business_code

            echo "Choose the field to update:"
            echo "1: Business name"
            echo "2: Address line 2"
            echo "3: Address line 3"
            echo "4: Post code"
            echo "5: Longitude"
            echo "6: Latitude"
            read -r element_choice

            case "$element_choice" in
                1) field=2 ;;
                2) field=3 ;;
                3) field=4 ;;
                4) field=5 ;;
                5) field=6 ;;
                6) field=7 ;;
                *)
                    echo "Invalid field choice."
                    continue
                    ;;
            esac

            echo "Enter the new value:"
            read -r new_value

            tmp_file="$(mktemp)"

            # Write the updated CSV to a temporary file before replacing the original.
            awk -F',' -v code="$business_code" -v field="$field" -v value="$new_value" '
                BEGIN { OFS = FS }
                NR == 1 { print; next }
                $1 == code {
                    $field = value
                    found = 1
                }
                { print }
                END {
                    if (!found) {
                        exit 2
                    }
                }
            ' "$file" > "$tmp_file"

            status=$?

            if [ "$status" -eq 0 ]; then
                mv "$tmp_file" "$file"
                echo "Business updated successfully."
            else
                rm "$tmp_file"
                echo "No business found with this ID."
            fi
            ;;

        4)
            less "$file"
            ;;

        5)
            echo "Enter output path, or press Enter to save as Businesses_saved.csv:"
            read -r save_path

            if [ -z "$save_path" ]; then
                save_path="$SCRIPT_DIR/Businesses_saved.csv"
            fi

            cp "$file" "$save_path"
            echo "File saved to: $save_path"
            ;;

        6)
            echo "Exiting."
            exit 0
            ;;

        *)
            echo "Invalid option."
            ;;
    esac
done