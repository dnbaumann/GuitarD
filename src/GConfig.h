/**
 * A few settings which allow more objects on the stack
 */

// Max amount of nodes in the graph
#define MAX_NODES 128

// Amounts of daw params to register at start since dynamic amounts are not well supported
#define MAX_DAW_PARAMS 256

// 8 Sockets for each in and output should be enough
#define MAX_NODE_SOCKETS 8

// Max amount of sockets a output socket can be connected to (not a hard limit, but weird things will happen)
#define MAX_SOCKET_CONNECTIONS 32

// Max amount of Parametercouplings for a node
#define MAX_NODE_PARAMETERS 16

// Meters are structs to share info about the dsp to the gui
#define MAX_NODE_METERS 8

// This is the name of a node if it wasn't overridden anywhere
#define DEFAULT_NODE_NAME "DEFAULTNODENAME"

/**
 * Nodes will allocated this much space for their buffers, larger chunks will be split up and processed normally
 * So making this smaller slightlu
 */
#define GUITARD_MAX_BUFFER 512

#define MIN_BLOCK_SIZE 64

#define MAX_UNDOS 8

// Distance in pixels from the cable the cursor needs to be within for the splice in to happen
#define SPLICEIN_DISTANCE 14

#define GUITARD_FLOAT_CONVOLUTION // Means we'll do float convolution since it allows sse

#ifdef __arm__ // No sse for arm obviously
  #undef GUITARD_SSE
#endif

#ifdef GUITARD_SSE
  #define FFTCONVOLVER_USE_SSE
#endif