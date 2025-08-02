# iCamera

A modular, multi-threaded camera system designed for embedded applications with real-time audio/video processing and network communication capabilities.

## Features

- **Multi-threaded Architecture**: Efficient task management with thread coordination
- **Inter-Task Communication**: Thread-safe mailbox system for message passing
- **Hardware Abstraction**: Modular design for audio, video, and network components
- **Real-time Processing**: Low-latency audio and video processing
- **Network Integration**: MQTT support for IoT connectivity
- **Graceful Shutdown**: Proper resource cleanup and thread management
- **Cross-platform**: Support for native and ARM cross-compilation

## Architecture Overview

iCamera uses a layered architecture with the following components:

- **Task Management Layer**: Centralized task coordination via `TaskManagers`
- **Communication Layer**: Thread-safe mailbox system for inter-task messaging
- **Application Tasks**: Specialized tasks for user input, message processing, audio, video, and networking
- **Hardware Abstraction Layer**: Device managers for audio, video, and network hardware

## Quick Start

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+)
- Threading library (pthread)
- SSL/TLS libraries (for MQTT)
- Paho MQTT C library

### Installation

1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   cd iCamera
   ```

42. Build the project:

```bash
# Build for Ubuntu 18.04 (native)
./scripts/build_ubuntu_18_04.sh

# Build for Luckfox (cross-compile)
./scripts/build_luckfox.sh
```

### Running the Application

```bash
# Run the application
./build/bin/iCamera
```

## Usage

### Basic Commands

Once the application is running, you can interact with it using the following commands:

- `msg <text>` - Send a text message
- `signal <number>` - Send a signal event
- `event <name> <payload>` - Send a custom event
- `quit` - Shutdown the application gracefully

### Example Session

```
iCamera starting...
[INFO] TaskManagers: Starting
[INFO] TaskManagers: Started task Sender
[INFO] TaskManagers: Started task Receiver
Enter command (msg <text> | signal <num> | event <name> <payload> | quit): msg Hello World
[Receiver] From: 2 To: 3
[Receiver] String: Hello World

Enter command (msg <text> | signal <num> | event <name> <payload> | quit): signal 5
[Receiver] From: 2 To: 3
[Receiver] SignalEvent: 5

Enter command (msg <text> | signal <num> | event <name> <payload> | quit): event test data
[Receiver] From: 2 To: 3
[Receiver] CustomEvent: test | data

Enter command (msg <text> | signal <num> | event <name> <payload> | quit): quit
[Receiver] From: 2 To: 3
[Receiver] SignalEvent: 0
[Receiver] Shutdown signal received. Exiting...
[INFO] TaskManagers: Stopping
[INFO] TaskManagers: Stopped task Sender
[INFO] TaskManagers: Stopped task Receiver
iCamera stopped
```

## Project Structure

```
iCamera/
├── docs/                          # Documentation
│   ├── ARCHITECTURE.md           # System architecture
│   └── API_REFERENCE.md          # API documentation
├── include/                       # Header files
│   └── paho/                     # MQTT library headers
├── lib/                          # Library files
├── scripts/                      # Build scripts
│   └── build_project.sh         # Main build script
├── source/                       # Source code
│   ├── app/                      # Application tasks
│   │   ├── aiotek_managers_task.cpp
│   │   ├── task_sender.cpp
│   │   └── task_receiver.cpp
│   ├── common/                   # Common utilities
│   │   └── aiotek_timer.cpp
│   ├── core/                     # Core system
│   │   └── aiotek_mailbox.cpp
│   ├── module/                   # Hardware modules
│   │   ├── audio/
│   │   ├── video/
│   │   └── network/
│   ├── utils/                    # Utility functions
│   │   ├── aiotek_log.cpp
│   │   └── aiotek_console.cpp
│   └── main.cpp                  # Main application
├── CMakeLists.txt               # CMake configuration
└── README.md                    # This file
```

## Configuration

### Build Configuration

The project supports multiple build configurations:

- **Native Build**: For development and testing on the host system
- **Cross-compilation**: For ARM-based embedded targets
- **Debug/Release**: Different optimization levels

### Runtime Configuration

Configuration can be modified through:

- **Hardware Settings**: Device parameters in respective manager classes
- **Network Settings**: MQTT broker configuration
- **Task Parameters**: Task-specific settings in `TaskManagers`

## Development

### Adding New Tasks

1. **Define the task function**:
   ```cpp
   void my_new_task() {
       while (true) {
           // Task logic here
           if (g_shutdown_requested) break;
       }
   }
   ```

2. **Register the task** in `TaskManagers` constructor:
   ```cpp
   tasks.push_back({"MyTask", my_new_task});
   ```

3. **Add message handling** if needed (optional)

### Adding New Message Types

1. **Extend the MailboxMessage variant**:
   ```cpp
   using MailboxMessage = std::variant<
       SignalEvent, ErrorEvent, CustomEvent,
       std::string, int, MyNewEvent  // Add new type
   >;
   ```

2. **Update message processing** in `task_receiver`

### Hardware Module Integration

1. **Implement device manager** following the existing pattern
2. **Create task wrapper** for the hardware module
3. **Add to task list** in `TaskManagers`
4. **Configure device parameters** as needed

## Testing

### Unit Testing

Individual components can be tested in isolation:

```bash
# Test mailbox functionality
# Test task management
# Test hardware modules
```

### Integration Testing

Test the complete system:

```bash
# Test task communication
# Test system startup/shutdown
# Test error handling
```

### Performance Testing

Monitor system performance:

```bash
# Measure message throughput
# Monitor resource usage
# Test under load
```

## Troubleshooting

### Common Issues

1. **Build Errors**:
   - Ensure all dependencies are installed
   - Check compiler version compatibility
   - Verify CMake configuration

2. **Runtime Errors**:
   - Check hardware device availability
   - Verify network connectivity (for MQTT)
   - Review log output for error details

3. **Thread Issues**:
   - Ensure proper thread cleanup
   - Check for race conditions
   - Verify thread safety

### Debug Mode

Enable debug logging:

```cpp
AIOTEK::Logger::setLevel(AIOTEK::LogLevel::DEBUG);
```

### Log Analysis

Review log output for:
- Task startup/shutdown messages
- Error events and codes
- Performance metrics
- System status information

## Contributing

1. **Fork the repository**
2. **Create a feature branch**
3. **Make your changes**
4. **Add tests** for new functionality
5. **Update documentation**
6. **Submit a pull request**

### Coding Standards

- Follow C++17 standards
- Use RAII for resource management
- Implement proper error handling
- Add comprehensive logging
- Maintain thread safety

## License

[Add your license information here]

## Support

For questions, issues, or contributions:

- **Issues**: Use the GitHub issue tracker
- **Documentation**: Check the `docs/` directory
- **API Reference**: See `docs/API_REFERENCE.md`
- **Architecture**: See `docs/ARCHITECTURE.md`

## Roadmap

### Planned Features

- **Plugin System**: Dynamic task loading
- **Web Interface**: Configuration and monitoring UI
- **Advanced Logging**: Structured logging with rotation
- **Metrics Collection**: Performance monitoring and analytics
- **High Availability**: Fault tolerance and recovery

### Performance Improvements

- **Load Balancing**: Task distribution optimization
- **Memory Optimization**: Reduced memory footprint
- **Latency Reduction**: Faster message processing
- **Scalability**: Multi-node support

---

*For detailed technical information, see the [Architecture Documentation](docs/ARCHITECTURE.md) and [API Reference](docs/API_REFERENCE.md).*
