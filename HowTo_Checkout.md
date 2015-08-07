## Quick Links ##

[Browse the code](http://code.google.com/p/nativeclient-sdk/source/browse#svn/trunk/src/native_client) |  [Recent changes](http://code.google.com/p/nativeclient-sdk/source/list) | [Git Mirror](https://chromium.googlesource.com/external/nativeclient-sdk/)

## Native Client SDK has moved ##

The Native Client SDK itself has moved to the chromium repository. Instructions for building and testing the Native Client SDK itself are located on the [chromium.org website](https://sites.google.com/a/chromium.org/dev/nativeclient/sdk/howto_buildtestsdk)

This repository is still used from some tools and applications that are  ancillary to the SDK itself and don't belong in the chromium repo.

## Prerequisites ##

You need to install [depot\_tools](http://dev.chromium.org/developers/how-tos/install-depot-tools) in order to use gclient.

## Checking Out ##

### Create a Project Directory ###

```
mkdir nativeclient-sdk
cd nativeclient-sdk
```

### Create a .gclient Configuration (git) ###
```
gclient config --name=src  https://chromium.googlesource.com/external/nativeclient-sdk.git
```

### Create a .gclient Configuration (svn) ###
```
gclient config http://nativeclient-sdk.googlecode.com/svn/trunk/src
```

### Sync to the repo ###
```
gclient sync
```

## Checking In (svn) ##

To submit code please upload it for review using:
```
gcl upload
```

After the review process commit to SVN using:
```
git cl upload
```

## Checking In (git) ##

To submit code please upload it for review using:
```
git cl upload
```

After the review process commit to SVN using:
```
git cl dcommit
```

Before commiting you will also need to configure git svn correctly:
```
git svn init https://nativeclient-sdk.googlecode.com/svn -Ttrunk/src
git config svn-remote.svn.fetch trunk/src:refs/remotes/origin/master
git svn fetch
```