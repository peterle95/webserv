#!/usr/bin/env python3.11
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)
import os
import sys
import cgi
import cgitb 

# Enable debugging (show errors in browser)
cgitb.enable()


# Only allow POST method
if os.environ.get("REQUEST_METHOD") != "POST":
    print("Error: Method not allowed")
    sys.exit(0)

# Get current working directory
cwd = os.getcwd()
print(f"Current working directory: {cwd}<br>")

# Define upload directory
upload_dir = os.path.join(cwd, "cgi_upload")

# Ensure upload directory exists
os.makedirs(upload_dir, exist_ok=True)

# Parse multipart form data
form = cgi.FieldStorage()

# Track if any file uploaded
file_uploaded = False

for field in form.keys():
    field_item = form[field]
    if field_item.filename:  # It's a file
        filename = os.path.basename(field_item.filename)
        filepath = os.path.join(upload_dir, filename)

        try:
            with open(filepath, "wb") as f:
                f.write(field_item.file.read())

            file_url = f"http://localhost:8080/cgi-bin/cgi_upload/{filename}"
            print(f"File uploaded successfully: <a href='{file_url}'>{filename}</a><br>")
            file_uploaded = True
        except Exception as e:
            print(f"Error: Unable to save file {filename} ({e})<br>")

if not file_uploaded:
    print("Error: No file uploaded<br>")

print('<p><a href="/cgi-bin/">Back to CGI Home</a></p>')
