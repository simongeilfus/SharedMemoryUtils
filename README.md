#SharedMemory Utilities

Initialize the server like this:
```c++
    sm::MessengerRef mMessages;
    mMessages = sm::Messenger::createServer( "SharedMemoryMessages" );
```

And as many clients as you want:
```c++
    sm::MessengerRef mMessages;
    mMessages = sm::Messenger::createClient( "SharedMemoryMessages" );
```
    
You can send and receive messages from both end
```c++
    sm::Message m = mMessages->createMessage( "Test" );
    m.addIntArg( randInt( 999999 ) );
    m.addFloatArg( randFloat() );
    m.addStringArg( toString( randVec2f() ) );
    
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