#!/usr/bin/env python3
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)
import cgi
import sys

# Parse form data
try:
    form = cgi.FieldStorage()
    apples = int(form.getfirst("apples", 0))
    price = float(form.getfirst("price", 0))
except Exception as e:
    print("Error type:", type(e).__name__)
    print("Error message:", str(e))
    sys.exit(1)

# Calculate total price
total_price = apples * price

# Print HTML response
print("""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Apple Order Result</title>
</head>
<body>
<h1>Apple Order Result</h1>
<p>Weight of Apples in kg: {}</p>
<p>Price per Kilo: {}</p>
<p>Total Price: {}</p>
<p><a href="/index.html/">Back to Home</a></p>
</body>
</html>""".format(apples, price, total_price))
