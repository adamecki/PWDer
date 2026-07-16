// pwrotocol - PWDer protocol for KeePass communication
// version 0.1
// let's see

#define PWROTOCOL_TIMEOUT_MS            10000
#define PWROTOCOL_IMPORT_MAGIC          "PWIMPORT"

#define PWROTOCOL_PING                  0x01
#define PWROTOCOL_GET_SALT              0x02
#define PWROTOCOL_GET_ITERATIONS        0x03
#define PWROTOCOL_FILE_IMPORT           0x04
#define PWROTOCOL_REQUEST_CONF          0x05

#define PWROTOCOL_CHUNK_SIZE            4096 // bytes

void pwrotocol_listen_and_respond();
