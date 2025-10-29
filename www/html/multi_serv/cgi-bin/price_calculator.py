#!/usr/bin/env python3
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)
import cgi
import sys

# Parse form data
try:
    form = cgi.FieldStorage()
    weight = float(form.getfirst("weight", 0))
    price_per_kg = float(form.getfirst("price", 0))
except Exception as e:
    print("Content-Type: text/html\n")
    print("Error:", str(e))
    sys.exit(1)

# Calculate total price
total_price = round(weight * price_per_kg, 2)

# Print HTML response
print("""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Price Calculation Result</title>
</head>
<body>
<h1>Price Calculation Result</h1>
<p><strong>Weight:</strong> {} kg</p>
<p><strong>Price per kg:</strong> €{:.2f}</p>
<p><strong>Total Price:</strong> €{:.2f}</p>
<p><a href="/index.html">Back to Home</a></p>
</body>
</html>""".format(weight, price_per_kg, total_price))