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
<body bgcolor="#f6fbff">
    <table align="center" bgcolor="#ffffff" cellpadding="30" style="border-radius:18px; box-shadow:0 8px 32px #82a4fd22;">
        <tr>
            <td>
                <h1 align="center" style="color:#2196F3;">Price Calculation Result</h1>
                <div style="text-align:center; color:#333; font-size:1.1em;">
                    <p><strong>Weight:</strong> {} kg</p>
                    <p><strong>Price per kg:</strong> €{:.2f}</p>
                    <p style="font-size:1.3em; color:#2196F3;"><strong>Total Price:</strong> €{:.2f}</p>
                </div>
                <div align="center" style="margin-top:30px;">
                    <a href="/index_multi.html" style="color:#2196F3; font-weight:bold; font-size:1.1em; text-decoration:none;">&#8592; Back to Home</a>
                </div>
            </td>
        </tr>
    </table>
</body>
</html>""".format(weight, price_per_kg, total_price))