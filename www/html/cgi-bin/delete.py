#!/usr/bin/env python3
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)
import os # For file operations and environment variables	
import sys # For reading stdin and exiting
import cgi # For CGI handling of python scripts

# Check request method
if os.environ.get("REQUEST_METHOD", "") == "POST":
    cwd = os.getcwd()
    upload_dir = os.path.join(cwd, "cgi_upload")

    # Read POST data from stdin
    post_data = sys.stdin.read().strip()

    # Extract filename (assuming same format: filename=<name>)
    if "filename=" in post_data:
        filename_to_delete = post_data.split("filename=", 1)[1].strip()
    else:
        filename_to_delete = ""

    print(f"Filename to delete: {filename_to_delete}<br>")

    # Validate filename
    if not filename_to_delete:
        print("Error: Filename not provided.<br>")
        print('<p><a href="/index.html/">Back to Home</a></p>')
        sys.exit(0)

    file_path = os.path.join(upload_dir, filename_to_delete)

    # Check if file exists
    if not os.path.exists(file_path):
        print(f"Error: File '{filename_to_delete}' does not exist.<br>")
        print('<p><a href="/index.html/">Back to Home</a></p>')
        sys.exit(0)

    # Attempt to delete the file
    try:
        os.remove(file_path)
        print(f"File '{filename_to_delete}' deleted successfully.<br>")
    except Exception as e:
        print(f"Error: Unable to delete file '{filename_to_delete}'. {e}<br>")

else:
    # Method not allowed
    print("Error: Method not allowed<br>")

print('<p><a href="/index.html">Back to Home</a></p>')
