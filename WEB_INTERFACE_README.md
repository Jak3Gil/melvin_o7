# Melvin Web Chat Interface

A local web interface for chatting with Melvin, built with a C HTTP server and modern web frontend.

## Quick Start

### Windows

1. **Build the server:**
   ```bash
   build.bat
   ```

2. **Run the server:**
   ```bash
   run.bat
   ```
   Or directly:
   ```bash
   melvin_server.exe
   ```

3. **Open your browser:**
   Navigate to `http://localhost:8080`

### Linux/Mac

1. **Build the server:**
   ```bash
   ./build.sh
   ```

2. **Run the server:**
   ```bash
   ./run.sh
   ```
   Or directly:
   ```bash
   ./melvin_server
   ```

3. **Open your browser:**
   Navigate to `http://localhost:8080`

## Features

- **Interactive Chat Interface**: Clean, modern UI for chatting with Melvin
- **Real-time Responses**: Send messages and receive Melvin's responses instantly
- **Learning System**: Melvin learns from each conversation, building patterns over time
- **System Status**: View error rate and system state in real-time
- **Local Only**: Everything runs locally on your PC - no internet required

## How It Works

1. **HTTP Server** (`melvin_server.c`): A lightweight C HTTP server that:
   - Serves the web interface files (HTML, CSS, JS)
   - Provides REST API endpoints (`/api/chat`, `/api/status`)
   - Maintains a persistent Melvin instance across all requests
   - Processes chat messages through Melvin's learning system

2. **Web Frontend** (`web/` directory):
   - `index.html`: Chat interface structure
   - `style.css`: Modern, responsive styling
   - `app.js`: Client-side logic for sending messages and displaying responses

3. **Melvin Integration**: Each chat message is processed through Melvin's `run_episode()` function, allowing the system to learn and respond based on patterns it has discovered.

## API Endpoints

### POST `/api/chat`
Send a message to Melvin.

**Request:**
```json
{
  "message": "Hello, Melvin!"
}
```

**Response:**
```json
{
  "response": "Melvin's response here",
  "error_rate": 0.500
}
```

### GET `/api/status`
Get current system status.

**Response:**
```json
{
  "status": "running",
  "error_rate": 0.500
}
```

## Building from Source

### Requirements
- GCC compiler (MinGW on Windows)
- Math library (`-lm` flag)
- Winsock2 (Windows) or POSIX sockets (Linux/Mac)

### Manual Build

**Windows:**
```bash
gcc -o melvin_server.exe melvin_server.c melvin.c -lm -lws2_32 -std=c99 -Wall
```

**Linux/Mac:**
```bash
gcc -o melvin_server melvin_server.c melvin.c -lm -std=c99 -Wall -pthread
```

## Troubleshooting

### Port Already in Use
If port 8080 is already in use, you can modify the `PORT` constant in `melvin_server.c` and rebuild.

### Server Won't Start
- Make sure you've compiled the server successfully
- Check that no firewall is blocking port 8080
- On Windows, ensure Winsock2 is available

### Browser Can't Connect
- Verify the server is running (check console output)
- Try `http://127.0.0.1:8080` instead of `localhost:8080`
- Check browser console for JavaScript errors

## File Structure

```
melvin_o7/
├── melvin.c              # Core Melvin implementation
├── melvin_server.c        # HTTP server with Melvin integration
├── web/                   # Web frontend files
│   ├── index.html        # Main chat interface
│   ├── style.css         # Styling
│   └── app.js            # Client-side JavaScript
├── build.bat             # Windows build script
├── build.sh              # Linux/Mac build script
├── run.bat               # Windows run script
└── run.sh                # Linux/Mac run script
```

## Notes

- The Melvin instance persists across all chat messages, allowing it to learn from the entire conversation
- Responses may be minimal initially as Melvin learns patterns
- The system processes messages as byte sequences, so it works with any text input
- All processing happens locally - no data is sent to external servers

## License

Same as the main Melvin project.
