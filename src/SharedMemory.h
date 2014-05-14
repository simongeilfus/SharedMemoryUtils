/*
 
 SharedMemory Utilities
 
 Copyright (c) 2014, Simon Geilfus - All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once


#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/offset_ptr.hpp>

namespace SharedMemory {
    
	
    namespace itp = boost::interprocess;
    
    typedef itp::managed_shared_memory                                      Msm;
    typedef Msm::allocator<char>::type                                      CharAllocator;
    typedef itp::basic_string<char, std::char_traits<char>, CharAllocator>  String;
    
    
    typedef enum _ArgType
    {
        TYPE_NONE,
        TYPE_INT32,
        TYPE_FLOAT,
        TYPE_STRING,
        TYPE_INDEXOUTOFBOUNDS
    } ArgType;
    
    
	class Arg {
	public:
		Arg( std::string value, const CharAllocator &a )
        : mValue( value.c_str(), a ), mType( TYPE_STRING ) {}
		Arg( int32_t value, const CharAllocator &a )
        : mValue( std::to_string( value ).c_str(), a ), mType( TYPE_INT32 ) {}
		Arg( float value, const CharAllocator &a )
        : mValue( std::to_string( value ).c_str(), a ), mType( TYPE_FLOAT ) {}
		
		//! returns the type
		ArgType getType() const { return mType; }
		
		//! returns the type
		void setType( ArgType type ) { mType = type; }
		
		//! returns value
		std::string get() const { return mValue.c_str(); }
        
		//! sets value
		void set( const char* value ) { mValue = value; }
		
	private:
        ArgType mType;
		String  mValue;
	};
    
    typedef itp::allocator<Arg, Msm::segment_manager>    ArgAllocator;
    typedef itp::vector<Arg,ArgAllocator>                ArgVector;
    
    
	class Message {
	public:
        //! Creates a message with a specific address
        Message( const std::string& address, Msm* msm );
        
        //! Copy Constructor
		//Message( const Message& other ){ copy ( other ); }
        //! Copy assignement
		Message& operator= ( const Message& other ) { return copy( other ); }
        
        //! Destructor
		~Message();
        
        //! Returns a copy of the message
		Message& copy( const Message& other );
        
        //! Clears message content
		void clear();
		
        //! Returns message address
		std::string getAddress() const { return mAddress.c_str(); }
		
        //! Returns number of arguments
		int         getNumArgs() const;
        //! Returns argument type by index
		ArgType     getArgType( int index ) const;
		
        //! Returns argument as an integer
		int32_t     getArgAsInt32( int index, bool typeConvert = false ) const;
        //! Returns argument as a float
		float       getArgAsFloat( int index, bool typeConvert = false ) const;
        //! Returns argument as a string
		std::string getArgAsString( int index, bool typeConvert = false ) const;
		
        //! Adds an integer argument
		void addIntArg( int32_t argument );
        //! Adds a float argument
		void addFloatArg( float argument );
        //! Adds a string argument
		void addStringArg( std::string argument );
		
	protected:
		String      mAddress;
		ArgVector   mArgs;
        
        Msm*        mMsm;
        
	};
    
    
    typedef itp::interprocess_mutex                                         Mutex;
    typedef itp::allocator<class Message, Msm::segment_manager>             MessageAllocator;
    typedef itp::deque<class Message,MessageAllocator>                      MessageDeque;
    typedef std::shared_ptr<class Messenger>                      MessengerRef;
    
    
    class MessageQueue {
    public:
        //! Creates a MessageQueue
        MessageQueue( const MessageAllocator &a )
        : mMessages( a ){}
        
        //! Adds a new messages at the back of the queue
        void push_back( const Message& message );
        
        //! Returns the first message in the queue
        const Message& front();
        //! Removes the first message in the queue
        void pop_front();
        
        //! Returns the first message address
        std::string getFrontAddress();
        
        //! Returns the number of messages available
        size_t size();
        
    protected:
        MessageDeque    mMessages;
        Mutex           mMutex;
    };
    
    
    class Messenger {
    public:
        //! Creates and returns a server side queue manager
        static MessengerRef createServer( const std::string &segmentName );
        //! Creates and returns a client side queue manager
        static MessengerRef createClient( const std::string &segmentName );
        
        ~Messenger();
        
        //! Returns wether the input queue has new messages
        bool hasMessageWaiting();
        
        //! Returns the number of available messages
        size_t getNumMessages();
        
        //! Returns the first message in the input queue
        Message getFrontMessage();
        
        //! Returns the address of the first message
        std::string getFrontAddress();
        
        //! Returns a new message with a specific address
        Message createMessage( const std::string& address );
        
        //! Adds a new message to the output queue
        void sendMessage( const Message& message );
        
    protected:
        
        Messenger( const std::string &segmentName, bool shouldReleaseMemory = false )
        : mSegmentName( segmentName ), mShouldReleaseMemory( shouldReleaseMemory ) {}
        
        Msm             mMsm;
        std::string     mSegmentName;
        MessageQueue*   mIn;
        MessageQueue*   mOut;
        bool            mShouldReleaseMemory;
    };
    
    
	
	class SharedMemoryExc : public std::exception {
	};
	class SharedMemoryExcInvalidArgumentType : public SharedMemoryExc {
	};
	class SharedMemoryExcOutOfBounds : public SharedMemoryExc {
	};
    
}

namespace sm = SharedMemory;