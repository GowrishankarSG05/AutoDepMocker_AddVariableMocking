/*******************************************************************************
* Copyright (C)
*******************************************************************************/

#ifndef STORAGE_HPP_
#define STORAGE_HPP_


class Storage {
public:
    static Storage& getInstance() {
        return s_thisPtr;
    }

    static Storage* s_thisPtr;

    MOCK_METHOD1(store, void(const char *));
};


#endif
