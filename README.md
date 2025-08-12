# webserv

<p align="center">
  <img src="https://github.com/mcombeau/mcombeau/blob/main/42_badges/webserve.png" alt="Webserv 42 project badge"/>
</p>

Here is a `README.md` file for your GitHub project, drawing on the information from the provided sources:

```markdown
# Webserv: A Non-Blocking HTTP Server from Scratch

## Project Overview

**Webserv** is a custom HTTP server implemented in **C++98**. This project aims to provide a deep understanding of how HTTP servers operate, focusing on core network aspects and **non-blocking behavior** to handle multiple concurrent connections efficiently. It demonstrates how to build a web server without external libraries (except standard C/C++ libraries).

The server's primary function is to **receive HTTP requests from clients (like web browsers), process these requests, and send appropriate HTTP responses back**.

## Features

The `webserv` project supports a variety of essential web server functionalities and adheres to the HTTP/1.1 protocol, with compatibility for standard web browsers.

### Core HTTP Functionality
*   **HTTP/1.1 Compliance**: The server generates responses in the format expected by web browsers, following rules specified in RFC documents (e.g., RFC 7230-7235, RFC 9110, RFC 9112).
*   **Supported HTTP Methods**: Implements the mandatory **GET**, **POST**, and **DELETE** methods.
    *   **GET**: Retrieves a representation of the target resource.
    *   **POST**: Processes enclosed content according to the resource's semantics, commonly used for sending form data or creating new resources.
    *   **DELETE**: Requests the removal of the association between the target resource and its current functionality.
*   **HTTP Response Status Codes**: Provides accurate HTTP status codes (e.g., 200 OK, 404 Not Found, 501 Not Implemented) in responses.
*   **Serving Static Websites**: Capable of serving fully static web pages.
*   **File Uploads**: Clients can upload files to the server, with configurable storage locations.
*   **Default Error Pages**: If no custom error page is provided, the server will display its own default error pages.

### Network and Performance
*   **Non-Blocking I/O**: The server is designed to be non-blocking at all times, preventing it from freezing or stalling when waiting for I/O operations.
*   **Single `poll()` (or Equivalent) for I/O**: Utilizes a single call to `poll()` (or `select()`, `epoll()`, `kqueue()`) to monitor all I/O operations between clients and the server simultaneously, including listening for new connections. This enables a single thread to manage multiple connections concurrently.
*   **TCP/IP Socket Programming**: Built on TCP sockets, ensuring reliable, ordered, and error-checked delivery of data.
    *   Covers socket creation, binding, listening for connections, accepting new connections, and sending/receiving messages.
*   **Handling Large Data (Chunking)**: Employs chunking to process large amounts of data in specified lengths, preventing the server from freezing.

### Configuration
The server's behavior is defined through a configuration file, inspired by NGINX server configuration format.
*   **Interface and Port Listening**: Define multiple `interface:port` pairs, allowing the server to listen on various ports and serve different content.
*   **Root Directory**: Set the root directory for requests.
*   **Index Files**: Specify default files to serve when a requested resource is a directory (e.g., `index.html`).
*   **Custom Error Pages**: Define specific error pages for different HTTP status codes (e.g., 404.html for 404 Not Found).
*   **Directory Listing (`autoindex`)**: Enable or disable automatic generation of directory listings.
*   **Allowed HTTP Methods**: Configure the list of accepted HTTP methods for specific URL routes.
*   **HTTP Redirection**: Define rules for HTTP redirection.
*   **Client Max Body Size**: Set the maximum allowed size for client request bodies.
*   **CGI Execution**: Support for Common Gateway Interface (CGI) execution based on file extensions (e.g., PHP, Python scripts).
    *   Handles environment variables, un-chunking requests, and processing CGI output.

## Getting Started

To get a copy of this project up and running on your local machine for development and testing, follow these steps.

### Prerequisites

*   A **Unix-based system** (macOS or Linux).
*   **`make`** utility for compilation.
*   **C++98 compiler** (e.g., `g++`).

### Installation

1.  **Clone the repository**:
    ```bash
    git clone <repository_url> # Placeholder: Replace <repository_url> with the actual URL of your repository.
    cd webserv # Change to the project directory
    ```
2.  **Compile the source code**:
    Use the provided `Makefile` to compile the server.
    ```bash
    make
    ```
    This will create the `webserv` executable in the project directory.

### Running the Server

The server requires a configuration file as an argument. You will need to create a configuration file (e.g., `config.nginx`) based on the supported directives mentioned above.

Example `config.nginx`:
```nginx
server {
    listen 8080;
    root html;
    index index.html;
    error_page 404 404.html;
    client_max_body_size 1MB;

    location / {
        allowed_methods GET POST DELETE;
        autoindex on;
    }
    location /upload {
        allowed_methods POST;
        upload_store /var/www/uploads; # Example: set your desired upload directory
    }
    location /cgi-bin {
        allowed_methods GET POST;
        cgi_extension .php .py; # Example: for PHP and Python CGI
    }
    location /redirect {
        return https://www.google.com;
    }
}
```
**To run the server**:
```bash
./webserv path/to/your/config.nginx
```

### Testing the Server

After running the server, you can test it using a web browser or tools like `curl` or `telnet`.

1.  **Open your web browser** and navigate to `localhost:8080` (or the port you configured in your `config.nginx`).
2.  Try accessing specific files like `localhost:8080/index.html`.
3.  Observe the server's output in the terminal where it's running. For instance, when visiting `localhost:8080/index.html` with the TCP server code, you might see the incoming GET request headers.
4.  If the server is configured with the example `char* hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!"` for its response, navigating to `localhost:8080` will display "Hello world!" in your browser.

For more advanced testing and debugging, you can compare your server's headers and behavior with those of NGINX. Stress testing is also recommended to ensure resilience.

## Technical Details

*   **Language**: C++98.
*   **Operating Systems**: Primarily developed for UNIX-based systems like macOS and Linux. TCP implementation may differ slightly on Windows, but the HTTP server logic remains language-independent.
*   **I/O Multiplexing**: The project demonstrates the use of `select()`, `poll()`, `epoll()` (Linux-specific), and `kqueue()` (macOS-specific) for handling multiple client connections concurrently and in a non-blocking manner. While `select()` is more portable, `poll()` and `epoll()` are often considered more efficient for a large number of file descriptors.
*   **RFC References**: The implementation is guided by HTTP RFCs, notably HTTP/1.1 specifications (RFC 7230-7235), which were later replaced by RFC 9110, RFC 9111, RFC 9112, RFC 9113, and RFC 9114. HTTP/1.0 (RFC 1945) is also a foundational reference.

## Acknowledgments

- Based on the 42 School curriculum project specifications in collaboration with [Shruti Gangal](https://github.com/shrutigangal) and [Alvin Varghese](https://github.com/Alvinvarghese)

