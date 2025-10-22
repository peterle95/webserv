#!/usr/bin/env python3
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)
import cgi
import sys

# Parse form data
try:
    form = cgi.FieldStorage()
    apples = float(form.getfirst("apples", 0))
    price = float(form.getfirst("price", 0))
except Exception as e:
    print("Error type:", type(e).__name__)
    print("Error message:", str(e))
    sys.exit(1)

# Calculate total price
total_price = round(apples * price, 2)

# Print HTML response
print("""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset=\"UTF-8\">
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
<title>Apple Order Result</title>
</head>
<body bgcolor=\"#f6fbff\">
    <h1 align=\"center\" style=\"color:#2196F3;\">Apple Order Result</h1>
    <p align=\"center\" style=\"color:#333;\">Weight of Apples in kg: {}<br>
    Price per Kilo: {}<br>
    Total Price: {}</p>
    <div align=\"center\">
        <a href=\"/index_multi.html\" style=\"color:#2196F3; font-weight:bold; text-decoration:none;\">&#8592; Back to Home</a>
    </div>
</body>
</html>""".format(apples, price, total_price))
