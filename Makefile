# C compiler we use
CXX = gcc

#CXXFLAGS = -Wall -pthread -Iinclude

CXXFLAGS = -pthread -Iinclude

# Output filename after compilation 
TARGET1 = nfs_client

TARGET2 = nfs_manager

TARGET3 = nfs_console

# Directory for object files
OBJ_DIR = obj

# Source files
SRC = src/ipc.c\
		src/client_threads.c\
		src/client_utils.c\
		src/manager_utils.c\
		src/queue.c\
		src/config.c\
		src/log.c\
		src/manager_threads.c
		
# Create object file names for each source file
OBJ = $(SRC:src/%.c=$(OBJ_DIR)/%.o)

# Main target
all: $(TARGET1) $(TARGET2) $(TARGET3)

# Build (nfs_client) executable
$(TARGET1): nfs_client.c $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET1) nfs_client.c $(OBJ)


$(TARGET2): nfs_manager.c $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET2) nfs_manager.c $(OBJ)


$(TARGET3): nfs_console.c $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET3) nfs_console.c $(OBJ)

# Build object files (without server.c)
$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and executables
clean:
	rm -rf $(TARGET1) $(TARGET2) $(TARGET3) $(OBJ_DIR)