// Creates an ES context to use for loading resources.
// Basically we pass this class to the SlideRenderers and they push up callbacks
// for the render loop

#pragma once


extern "C"
{
#include "bcm_host.h"
#include "ilclient/ilclient.h"
}


// Singleton pattern used:
// https://stackoverflow.com/questions/1008019/c-singleton-design-pattern

class PiOMXUser{
public:

    static PiOMXUser& getInstance()
    {
        static PiOMXUser instance; // Guaranteed to be destroyed.
                                        // Instantiated on first use.
        return instance;
    }

    ~PiOMXUser();

     ILCLIENT_T     *getILClient() {
        return client;
    }

private: 

	PiOMXUser();
    ILCLIENT_T     *client;


private:

    // C++ 11
    // =======
    // We can use the better technique of deleting the methods
    // we don't want.
public:
    PiOMXUser(PiOMXUser const&)  = delete;
    void operator=(PiOMXUser const&)  = delete;

    // Note: Scott Meyers mentions in his Effective Modern
    //       C++ book, that deleted functions should generally
    //       be public as it results in better error messages
    //       due to the compilers behavior to check accessibility
    //       before deleted status

};