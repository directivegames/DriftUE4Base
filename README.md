# DriftUE4Base
Base module for Drift UE4 Integration

This suite of modules and plugin offers a quick and convenient way for you to integrating Drift into your
Unreal Engine 4 based game. It offers both a complete UE4 friendly C++ API, and convenient blueprint access
to basic features.

There's also an OnlineSubsystem implementation, but it still doesn't cover all the features.

Only a small subset of features are available from blueprints. This is an area we may extend if the demand is high enough.

## Installation

Pending updated documentation

Open the Project Settings pane and go to the Pugins/Drift section. Enter the relevant data there.

Somehwere during startup, you'll want to authenticate the player. In the editor and by default it'll
use automatically created users and passwords that are stored on the local computer. This is currently
only secure for iOS, where it uses the KeyChain. Secure implementations for Windows and other platforms
will come later.

The editor supports multiple PIE sessions, which will all get their own user/password combination.

```
void MyClass::Login()
{
	// Get an instance of the drift API.
	if (auto drift = FDriftWorldHelper(GetWorld()).GetInstance())
	{
		// Register a callback for handling when the player login is complete
		drift->OnPlayerAuthenticated().AddUObject(this, &MyClass::HandlePlayerAuthenticated);
		// Begin authentication
		drift->AuthenticatePlayer();
	}
}

void MyClass::HandlePlayerAuthenticated(bool success, const FPlayerAuthenticatedInfo& info)
{
	if (success)
	{
		// Show player UI, load a new level, or whatever you need
	}
	else
	{
		// Something went wrong, details are in `info.error`
		UE_LOG(LogTemp, Error, TEXT("Failed to log on: %s"), *info.error);
	}
}
```

## User Sessions

## Error Handling

Once a session has been established, the things that can tear down the session are:
* The network connection goes down.
* The client fails to heartbeat within the session timeout limit. Hearbeats are handled by the plugin,
  you don't need to do anything.
* The user signs in to the same game using a second client, in which case the session is usurped, and
  no further calls from the initial client will be accepted.
* The user calls disconnect.
* The API key or key/version combination is being revoked on the server.
* Generally, failure to complete other operations don't cause a disconnect, and if the game handles the
  errors appropriately, the calls can be retried later.

Any other errors, occuring from calling non authentication endpoints will generate the appropriate error
callbacks, but won't tear down the session. It's up to each game to decide how to deal with each type
of error.

# Usage

## Dedicated Server

When using the `OnlineSubsystemDrift`, override `RegisterServer` in your `GameSession` class and call `CreateSession`
with the relevant paramters to advertise the game session. You'll need to override `GetGameSessionClass` in your
custom `GameMode` class to return the new `GameSession` class.
To simplify the remaining game logic, it's often convenient to implement the login and session registration in
an empty "Login" map, which loads the real game map after successfully registering the session.
