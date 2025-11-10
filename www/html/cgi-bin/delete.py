#!/usr/bin/env python3
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)
import os # For file operations and environment variables	
import sys # For reading stdin and exiting
import cgi # For CGI handling of python scripts
import urllib.parse

# Check request method
if os.environ.get("REQUEST_METHOD", "") == "POST":
    cwd = os.getcwd()
    upload_dir = os.path.join(cwd, "cgi_upload")
    
    
    # Read POST data from stdin
    post_data = sys.stdin.read().strip()

    # Parse form-encoded POST body into parameters
    params = urllib.parse.parse_qs(post_data, keep_blank_values=True)
    filename_to_delete = params.get("filename", [""])[0].strip()

    # Values from parse_qs are already percent-decoded.
    if not filename_to_delete:
        filename_to_delete = ""

    print(f"Filename to delete: {filename_to_delete}<br>")

    # Validate filename
    if not filename_to_delete:
        print("Error: Filename not provided.<br>")
        print('<p><a href="/index.html">Back to Home</a></p>')
        sys.exit(0)

    if '..' in filename_to_delete or '/' in filename_to_delete or '\\' in filename_to_delete:
        print("Error: Invalid filename.<br>")
        print('<p><a href="/index.html">Back to Home</a></p>')
        sys.exit(0)   

    file_path = os.path.join(upload_dir, filename_to_delete)

    if not os.path.abspath(file_path).startswith(os.path.abspath(upload_dir)):
        print("Error: Invalid filename path.<br>")
        print('<p><a href="/index.html">Back to Home</a></p>')
        sys.exit(0)

    # Check if file exists
    if not os.path.exists(file_path):
        print(f"Error: File '{filename_to_delete}' does not exist.<br>")
        print('<p><a href="/index.html">Back to Home</a></p>')
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