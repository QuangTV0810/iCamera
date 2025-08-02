# iCamera API Reference

## Namespace: AIOTEK

### Core Classes

#### TaskManagers

**Header**: `source/app/aiotek_managers_task.hpp`

**Purpose**: Central task coordinator and thread manager for the iCamera system.

**Constructor**:
```cpp
TaskManagers();
```
Creates a new TaskManagers instance and registers default tasks (Sender, Receiver).

**Destructor**:
```cpp
~TaskManagers();
```
Automatically stops all tasks and joins threads.

**Public Methods**:

```cpp
bool start();
```
Starts all registered tasks in separate threads.
- **Returns**: `true` if successful, `false` if already running
- **Thread Safety**: Not thread-safe

```cpp
void stop();
```
Stops all tasks and joins their threads.
- **Thread Safety**: Not thread-safe

```cpp
bool state() const;
```
Checks if the task manager is currently running.
- **Returns**: `true` if running, `false` otherwise
- **Thread Safety**: Thread-safe

**Private Methods**:

```cpp
void run();
```
Internal method that runs the main task manager loop.

```cpp
void processManagers();
```
Internal method for processing manager operations.

**Global Instance**:
```cpp
extern TaskManagers managers;
```
Global instance of TaskManagers accessible throughout the application.

---

#### Mailbox

**Header**: `source/core/aiotek_mailbox.hpp`

**Purpose**: Thread-safe message queue for inter-task communication.

**Public Methods**:

```cpp
void send(const MailboxEnvelope& env);
```
Sends a message envelope to the mailbox.
- **Parameters**: `env` - Message envelope to send
- **Thread Safety**: Thread-safe

```cpp
MailboxEnvelope receive();
```
Receives a message from the mailbox (blocking).
- **Returns**: Next message envelope from the queue
- **Thread Safety**: Thread-safe
- **Blocking**: Yes, waits until message is available

```cpp
std::optional<MailboxEnvelope> try_receive();
```
Attempts to receive a message without blocking.
- **Returns**: Message envelope if available, `std::nullopt` otherwise
- **Thread Safety**: Thread-safe
- **Blocking**: No

**Global Instance**:
```cpp
extern Mailbox g_mailbox;
```
Global mailbox instance for inter-task communication.

---

### Data Structures

#### TaskEntry

**Header**: `source/app/aiotek_managers_task.hpp`

**Purpose**: Represents a registered task in the system.

```cpp
struct TaskEntry {
    std::string name;           // Task identifier
    std::function<void()> func; // Task function pointer
    std::thread thread;         // Associated thread
};
```

#### MailboxEnvelope

**Header**: `source/core/aiotek_mailbox.hpp`

**Purpose**: Message envelope for mailbox communication.

```cpp
struct MailboxEnvelope {
    TaskID sender;        // Source task identifier
    TaskID receiver;      // Destination task identifier
    MailboxMessage payload; // Message content
};
```

#### MailboxMessage

**Header**: `source/core/aiotek_mailbox.hpp`

**Purpose**: Variant type for different message types.

```cpp
using MailboxMessage = std::variant<
    SignalEvent,    // Signal events
    ErrorEvent,     // Error events
    CustomEvent,    // Custom events
    std::string,    // String messages
    int            // Integer messages
>;
```

---

### Event Types

#### SignalEvent

**Header**: `source/core/aiotek_mailbox.hpp`

```cpp
struct SignalEvent {
    int signal; // Signal number
};
```

#### ErrorEvent

**Header**: `source/core/aiotek_mailbox.hpp`

```cpp
struct ErrorEvent {
    int code;           // Error code
    std::string message; // Error message
};
```

#### CustomEvent

**Header**: `source/core/aiotek_mailbox.hpp`

```cpp
struct CustomEvent {
    std::string name;    // Event name
    std::string payload; // Event payload
};
```

---

### Enumerations

#### TaskID

**Header**: `source/core/aiotek_mailbox.hpp`

**Purpose**: Identifies different tasks in the system.

```cpp
enum class TaskID {
    Unknown = 0,
    Console,
    Sender,
    Receiver,
    Audio,
    Video,
    Managers,
    MQTT
};
```

**Utility Functions**:

```cpp
const char* TaskIDToString(TaskID id);
```
Converts TaskID to string representation.
- **Parameters**: `id` - TaskID to convert
- **Returns**: String representation of the TaskID

```cpp
void PrintAllTaskIDs();
```
Prints all available TaskIDs to console.

---

### Task Functions

#### task_sender

**Header**: `source/app/task_sender.cpp`

**Purpose**: Handles user input and sends messages to the mailbox.

**Signature**:
```cpp
void task_sender();
```

**Commands**:
- `msg <text>` - Sends text message
- `signal <num>` - Sends signal event
- `event <name> <payload>` - Sends custom event
- `quit` - Initiates shutdown

**Thread Safety**: Not thread-safe (runs in dedicated thread)

---

#### task_receiver

**Header**: `source/app/task_receiver.cpp`

**Purpose**: Processes incoming messages from the mailbox.

**Signature**:
```cpp
void task_receiver();
```

**Capabilities**:
- Processes all message types (SignalEvent, ErrorEvent, CustomEvent, String, Int)
- Handles shutdown signals
- Displays message information

**Thread Safety**: Not thread-safe (runs in dedicated thread)

---

### Timer Class

#### Timer

**Header**: `source/common/aiotek_timer.hpp`

**Purpose**: High-precision timer for performance measurement.

**Public Methods**:

```cpp
void start();
```
Starts the timer.

```cpp
void stop();
```
Stops the timer.

```cpp
double getElapsedSeconds() const;
```
Gets elapsed time in seconds.
- **Returns**: Elapsed time as double

```cpp
std::string getElapsedString() const;
```
Gets elapsed time as formatted string.
- **Returns**: Formatted time string

---

### Logging System

#### Logger

**Header**: `source/utils/aiotek_log.hpp`

**Purpose**: Centralized logging system.

**Log Levels**:
```cpp
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};
```

**Public Methods**:

```cpp
void setLevel(LogLevel level);
```
Sets the current log level.

```cpp
void debug(const std::string& message);
void info(const std::string& message);
void warning(const std::string& message);
void error(const std::string& message);
```
Log messages at different levels.

**Macros**:
```cpp
AIOTEK_LOG_DEBUG(message)
AIOTEK_LOG_INFO(message)
AIOTEK_LOG_WARNING(message)
AIOTEK_LOG_ERROR(message)
```

---

### Console Input

#### Console Functions

**Header**: `source/utils/aiotek_console.hpp`

```cpp
std::string aiotek_console_readline(const std::string& prompt);
```
Reads a line from console with prompt.
- **Parameters**: `prompt` - Prompt to display
- **Returns**: User input string

---

### Hardware Managers

#### AudioManager

**Header**: `source/module/audio/aiotek_audio.hpp`

**Purpose**: Manages audio device operations.

**Key Methods**:
```cpp
bool initialize();
bool startCapture();
void stopCapture();
bool isCapturing() const;
std::vector<uint8_t> getAudioData();
bool processAudio(const std::vector<uint8_t>& data);
```

#### VideoManager

**Header**: `source/module/video/aiotek_video.hpp`

**Purpose**: Manages video device operations.

**Key Methods**:
```cpp
bool initialize(const VideoConfig& config);
bool startCapture();
void stopCapture();
bool isCapturing() const;
VideoFrame getFrame();
bool hasFrame() const;
bool processFrame(const VideoFrame& frame);
```

#### MQTTManager

**Header**: `source/module/network/mqtt/aiotek_mqtt.hpp`

**Purpose**: Manages MQTT network communication.

**Key Methods**:
```cpp
int32_t connect();
void disconnect();
int32_t publish(const std::string& topic, const nlohmann::json& payload);
int32_t subscribe(const std::string& topic);
bool isConnected() const;
```

---

### Configuration

#### MQTTConfig

**Header**: `source/module/network/mqtt/aiotek_mqtt.hpp`

```cpp
struct MQTTConfig {
    std::string broker = "broker.hivemq.com";
    uint32_t port = 1883;
    std::string clientId = "";
    std::string username = "";
    std::string password = "";
    std::string topic = "";
    uint32_t keepAlive = 60;
    bool ssl = true;
};
```

#### VideoConfig

**Header**: `source/module/video/aiotek_video.hpp`

```cpp
struct VideoConfig {
    int width = 1920;
    int height = 1080;
    int fps = 30;
    std::string format = "YUYV";
    std::string device = "/dev/video0";
};
```

---

### Error Handling

#### Exception Types

The system uses standard C++ exceptions:
- `std::runtime_error` - Runtime errors
- `std::invalid_argument` - Invalid arguments
- `std::system_error` - System-level errors

#### Error Codes

Common error codes:
- `0` - Success
- `-1` - General error
- `-2` - Invalid configuration
- `-3` - Device not available
- `-4` - Network error

---

### Thread Safety

#### Thread-Safe Components
- `Mailbox` - All methods are thread-safe
- `Logger` - All methods are thread-safe
- `Timer` - Methods are not thread-safe (use per-thread instances)

#### Thread-Unsafe Components
- `TaskManagers` - Public methods are not thread-safe
- Task functions - Run in dedicated threads
- Hardware managers - Use per-instance

---

### Memory Management

#### Smart Pointers
The system uses `std::unique_ptr` for resource management:
```cpp
std::unique_ptr<VideoDevice> device;
std::unique_ptr<AudioDevice> audioDevice;
```

#### RAII Pattern
All resources are managed using RAII:
- Threads are automatically joined in destructors
- File handles are automatically closed
- Network connections are automatically closed

---

### Build Configuration

#### CMake Variables
- `CMAKE_BUILD_TYPE` - Debug/Release
- `CROSS_COMPILE` - Enable cross-compilation
- `CMAKE_CXX_STANDARD` - C++17

#### Compiler Flags
- `-Wall -Wextra` - Warning flags
- `-fPIC` - Position independent code
- `-O3` - Release optimization
- `-g -O0` - Debug symbols

---

*This API reference covers the core functionality of the iCamera system. For implementation details, refer to the source code and architecture documentation.* 