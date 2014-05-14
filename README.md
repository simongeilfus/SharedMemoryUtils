#SharedMemory Utilities

####Server/Clients Messages System
Initialize the server like this:
```c++
sm::MessengerRef mMessages;
mMessages = sm::Messenger::createServer( "SharedMemoryName" );
```

And as many clients as you want:
```c++
sm::MessengerRef mMessages;
mMessages = sm::Messenger::createClient( "SharedMemoryName" );
```
    
You can send and receive messages from both end. The only difference between server and clients is that the server take care of creating and destroying the shared memory segment.
```c++
sm::Message m = mMessages->createMessage( "Test" );
m.addIntArg( 123456 );
m.addFloatArg( 0.987654f );
m.addStringArg( "Hello Shared Memory!" );

mMessages->sendMessage( m );
```
```c++
if( mMessages->hasMessageWaiting() ){
    sm::Message m = mMessages->getFrontMessage();
    cout << m.getAddress() << endl;
    for( int i = 0; i < m.getNumArgs(); i++ ){
        cout << '\t' << m.getArgAsString( i ) << endl;
    }
}
```
####Smart Pointers
Create your shared_ptr like this:
```c++
sm::shared_memory_object::remove( "SharedMemory" );
    
sm::managed_shared_memory sharedMemory  = sm::managed_shared_memory( sm::itp::create_only, "SharedMemory", sizeof(Object) );
sm::shared_ptr<Object>                  = sm::make_shared( mSharedMemory.construct<Test>("SharedObject")(), sharedMemory );
```
And find them on the other side like this:
```c++
sm::managed_shared_memory sharedMemory  = sm::managed_shared_memory( sm::itp::open_only, "SharedMemory" );
sm::shared_ptr<Object>                  = sm::make_shared( mSharedMemory.find<Test>("SharedObject").first, sharedMemory );
```
