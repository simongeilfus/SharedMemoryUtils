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

#include "SharedMemory.h"


namespace SharedMemory {
    
    
    //! Creates a message with an address
    Message::Message( const std::string& address, Msm* msm )
    : mAddress( address.c_str(), CharAllocator( msm->get_allocator<char>() ) ),
    mArgs( ArgAllocator( msm->get_segment_manager() ) ),
    mMsm( msm )
    {
    }
    
    //! Destructor
    Message::~Message()
    {
        clear();
    }
    
    //! Returns a copy of the message
    Message& Message::copy( const Message& other )
    {
        mAddress = other.mAddress;
        
        for ( int i=0; i<(int)other.mArgs.size(); ++i ){
            ArgType argType = other.getArgType( i );
            if ( argType == TYPE_INT32 )
                mArgs.push_back( Arg( other.getArgAsInt32( i ), CharAllocator( mMsm->get_allocator<char>() ) ) );
            else if ( argType == TYPE_FLOAT )
                mArgs.push_back( Arg( other.getArgAsFloat( i ), CharAllocator( mMsm->get_allocator<char>() ) ) );
            else if ( argType == TYPE_STRING )
                mArgs.push_back( Arg( other.getArgAsString( i ), CharAllocator( mMsm->get_allocator<char>() ) ) );
            else {
                throw SharedMemoryExcInvalidArgumentType();
            }
        }
        
        return *this;
    }
    
    //! Clears message content
    void Message::clear()
    {
        mArgs.clear();
        mAddress = "";
    }
    
    
    //! Returns number of arguments
    int Message::getNumArgs() const
    {
        return (int)mArgs.size();
    }
    //! Returns argument type by index
    ArgType Message::getArgType( int index ) const
    {
        if (index >= (int)mArgs.size()){
            throw SharedMemoryExcOutOfBounds();
        }else {
            return mArgs[index].getType();
        }
    }
    
    //! Returns argument as an integer
    int32_t Message::getArgAsInt32( int index, bool typeConvert ) const
    {
        if (getArgType(index) != TYPE_INT32){
            if( typeConvert && (getArgType(index) == TYPE_FLOAT) )
                return std::stoi( mArgs[index].get() );
            else
                throw SharedMemoryExcInvalidArgumentType();
        }else
            return std::stoi( mArgs[index].get() );
    }
    //! Returns argument as a float
    float Message::getArgAsFloat( int index, bool typeConvert ) const
    {
        if (getArgType(index) != TYPE_FLOAT){
            if( typeConvert && (getArgType(index) == TYPE_INT32) )
                return std::stof( mArgs[index].get() );
            else
                throw SharedMemoryExcInvalidArgumentType();
        }else
            return std::stof( mArgs[index].get() );
    }
    //! Returns argument as a string
    std::string Message::getArgAsString( int index, bool typeConvert ) const
    {
        return mArgs[index].get();
    }
    
    //! Adds an integer argument
    void Message::addIntArg( int32_t argument )
    {
        mArgs.push_back( Arg( argument, CharAllocator( mMsm->get_allocator<char>() ) ) );
    }
    //! Adds a float argument
    void Message::addFloatArg( float argument )
    {
        mArgs.push_back( Arg( argument, CharAllocator( mMsm->get_allocator<char>() ) ) );
    }
    //! Adds a string argument
    void Message::addStringArg( std::string argument )
    {
        mArgs.push_back( Arg( argument, CharAllocator( mMsm->get_allocator<char>() ) ) );
    }
    
    
    
    
    //! Adds a new messages at the back of the queue
    void MessageQueue::push_back( const Message& message )
    {
        itp::scoped_lock<itp::interprocess_mutex> lock( mMutex );
        mMessages.push_back( message );
    }
    
    //! Returns the first message in the queue
    const Message& MessageQueue::front()
    {
        itp::scoped_lock<itp::interprocess_mutex> lock( mMutex );
        return mMessages.front();
    }
    //! Removes the first message in the queue
    void MessageQueue::pop_front()
    {
        itp::scoped_lock<itp::interprocess_mutex> lock( mMutex );
        mMessages.pop_front();
    }
    
    //! Returns the first message address
    std::string MessageQueue::getFrontAddress()
    {
        itp::scoped_lock<itp::interprocess_mutex> lock( mMutex );
        
        if( size() ){
            return mMessages.front().getAddress();
        }
        else {
            return "";
        }
    }
    
    //! Returns the number of messages available
    size_t MessageQueue::size()
    {
        itp::scoped_lock<itp::interprocess_mutex> lock( mMutex );
        return mMessages.size();
    }
    
    
    
    
    
    //! Creates and returns a server side queue manager
    MessengerRef Messenger::createServer( const std::string &segmentName )
    {
        MessengerRef ref;
        
        try {
            // Erase previous shared memory
            itp::shared_memory_object::remove( segmentName.c_str() );
            
            // Create manager instance
            ref = MessengerRef( new Messenger( segmentName, true ) );
            
            // Create a shared memory object.
            ref->mMsm    = Msm( itp::create_only, segmentName.c_str(), 65536 );
            
            // Create input / output queues
            ref->mIn     = ref->mMsm.find_or_construct<MessageQueue>( "Clients" )( MessageAllocator( ref->mMsm.get_segment_manager() ) );
            ref->mOut    = ref->mMsm.find_or_construct<MessageQueue>( "Server" )( MessageAllocator( ref->mMsm.get_segment_manager() ) );
        }
        catch( itp::interprocess_exception &exc ){
            std::cout << exc.what() << std::endl;
        }
        
        return ref;
    }
    
    //! Creates and returns a client side queue manager
    MessengerRef Messenger::createClient( const std::string &segmentName )
    {
        MessengerRef ref;
        
        try {
            // Create manager instance
            ref = MessengerRef( new Messenger( segmentName ) );
            
            // Create a shared memory object.
            ref->mMsm    = Msm( itp::open_only, segmentName.c_str() );
            
            // Find input / output queues
            ref->mIn     = ref->mMsm.find<MessageQueue>( "Server" ).first;
            ref->mOut    = ref->mMsm.find<MessageQueue>( "Clients" ).first;
        }
        catch( itp::interprocess_exception &exc ){
            std::cout << exc.what() << std::endl;
        }
        
        return ref;
    }
    
    Messenger::~Messenger()
    {
        if( mShouldReleaseMemory ){
            // Erase shared memory
            itp::shared_memory_object::remove( mSegmentName.c_str() );
        }
    }
    
    //! Returns wether the input queue has new messages
    bool Messenger::hasMessageWaiting()
    {
        if( mIn == NULL ) return false;
        else return mIn->size();
    }
    
    //! Returns the number of available messages
    size_t Messenger::getNumMessages()
    {
        if( mIn == NULL ) return 0;
        else return mIn->size();
    }
    
    //! Returns the first message in the input queue
    Message Messenger::getFrontMessage()
    {
        Message m( "", &mMsm );
        if( mIn != NULL ){
            m.copy( mIn->front() );
            mIn->pop_front();
        }
        return m;
    }
    
    //! Returns the address of the first message
    std::string Messenger::getFrontAddress()
    {
        if( mIn == NULL ) return "";
        else return mIn->getFrontAddress();
    }
    
    //! Returns a new message with a specific address
    Message Messenger::createMessage( const std::string& address )
    {
        return Message( address, &mMsm );
    }
    
    //! Adds a new message to the output queue
    void Messenger::sendMessage( const Message& message )
    {
        if( mOut == NULL ) return;
        else {
            mOut->push_back( message );
        }
    }
    
}