#!/usr/bin/python3
import sys
import os
import urllib.parse

def parse_post_data(content_length):
    try:
        length = int(content_length)
    except (ValueError, TypeError):
        return {}

    post_data = sys.stdin.read(length)
    parsed_data = urllib.parse.parse_qs(post_data)

    data = {}
    for key, value_list in parsed_data.items():
        data[key] = value_list[0]

    return data

def calculate(num1_str, operation, num2_str):
    try:
        num1 = int(num1_str.split('.')[0])
        num2 = int(num2_str.split('.')[0])
    except ValueError:
        return "Error: Please enter valid integers."

    if operation == 'add':
        result = num1 + num2
        symbol = "+"
    elif operation == 'subtract':
        result = num1 - num2
        symbol = "-"
    elif operation == 'multiply':
        result = num1 * num2
        symbol = "*"
    elif operation == 'divide':
        if num2 == 0:
            return "Error: Division by zero is not allowed!"
        result = num1 // num2
        symbol = "/"
    else:
        return "Error: Invalid operation."

    return f"{num1} {symbol} {num2} = {result}"

print("Content-type: text/html\r\n\r")

content_length = os.environ.get("CONTENT_LENGTH")
form_data = parse_post_data(content_length)

num1 = form_data.get('num1', '0')
num2 = form_data.get('num2', '0')
operation = form_data.get('operation', '')

calculation_result = calculate(num1, operation, num2)

print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Calculation Result</title>
    <style>
        body {{ font-family: Arial, sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; background-color: #f4f4f9; }}
        .container {{ background: white; padding: 40px; border-radius: 10px; box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1); text-align: center; }}
        h1 {{ color: #28a745; margin-bottom: 20px; }}
        .sonuc {{ font-size: 2em; font-weight: bold; margin-bottom: 20px; }}
        #backButton {{ background-color: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; text-decoration: none; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>Calculation Result</h1>
        <div class="sonuc">{calculation_result}</div>
        <a href="/calculator">
            <button id="backButton">New Calculation</button>
        </a>
    </div>
</body>
</html>
""")
