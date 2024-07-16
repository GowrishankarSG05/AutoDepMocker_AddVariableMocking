#include "External.hpp" // Content of External.hpp to be mocked

class MySourceClass {
public:
    MySourceClass() {
        storageHandler.store("Book");
    }

    StorageClass storageHandler;
};