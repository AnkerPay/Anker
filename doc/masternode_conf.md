Masternode Setup Guide
=======================
## Introduction ##

This guide is for a single masternode, on a Ubuntu 16.04 64bit server (VPS) running headless and will be controlled from the wallet on your local computer (Control wallet). The wallet on the the VPS will be referred to as the Remote wallet.
You will need your server details for progressing through this guide.

First the basic requirements:

 * 4,000 ANK
 * A main computer (Your everyday computer) – This will run the control wallet, hold your collateral 4,000 ank and can be turned on and off without affecting the masternode.
 * Masternode Server (VPS – The computer that will be on 24/7)
 * A unique IP address for your VPS / Remote wallet

(For security reasons, you’re are going to need a different IP for each masternode you plan to host)

The basic reasoning for these requirements is that, you get to keep your ANK in your local wallet and host your masternode remotely, securely.
## Configuration ##

Note: The auto zANK minter should be disabled during this setup to prevent autominting of your masternode collateral. BEFORE unlocking your wallet, you can disable autominting in the control wallet option menu.

1) Using the control wallet, enter the debug console (Tools > Debug console) and type the following command:
```
masternode genkey
```
 (This will be the masternode’s privkey. We’ll use this later…)
2) Using the control wallet still, enter the following command:
```
getaccountaddress chooseAnyNameForYourMasternode
```
3) Still in the control wallet, send 4,000 ANK to the address you generated in step 2 (Be 100% sure that you entered the address correctly. You can verify this when you paste the address into the “Pay To:” field, the label will autopopulate with the name you chose”, also make sure this is exactly 4,000 ANK; No less, no more.)
– Be absolutely 100% sure that this is copied correctly. And then check it again. We cannot help you, if you send 4,000 ANK to an incorrect address.

4) Still in the control wallet, enter the command into the console:
```
masternode outputs
```
 (This gets the proof of transaction of sending 4,000)

5) Still on the main computer, go into the ANK data directory, by default in Windows it’ll be
```
%Appdata%/ANK
```
or Linux
```
~
```

Find masternode.conf and add the following line to it:
```
 <Name of Masternode(Use the name you entered earlier for simplicity)> <Unique IP address>:12365 <The result of Step 1> <Result of Step 4> <The number after the long line in Step 4>
```
```
Example: MN1 31.14.135.27:12365 892WPpkqbr7sr6Si4fdsfssjjapuFzAXwETCrpPJubnrmU6aKzh c8f4965ea57a68d0e6dd384324dfd28cfbe0c801015b973e7331db8ce018716999 1
```
Substitute it with your own values and without the “<>”s
VPS Remote wallet install

7) Install the latest version of the ANK wallet onto your masternode. The lastest version can be found here: https://github.com/fauxe/Anker

    Note: If this is the first time running the wallet in the VPS, you’ll need to attempt to start the wallet 
    ```
    ./ankerd
    ```
     this will place the config files in your ~/.anker data directory
    ```
    press CTRL+C
    ```
    to exit / stop the wallet

8) Now on the masternodes, find the ANK data directory here.(Linux: ~/.anker)
```
cd ~/.anker
```
9) Open the anker.conf by typing 
vi anker.conf
 then press i to go into insert mode and make the config look like this:
```
     rpcuser=long random username
     rpcpassword=longer random password
     rpcallowip=127.0.0.1
     server=1
     daemon=1
     logtimestamps=1
     maxconnections=256
     masternode=1
     externalip=your unique public ip address
     masternodeprivkey=Result of Step 1
```
Make sure to replace rpcuser and rpcpassword with your own.

## Start your masternode ##

11) Now, you need to finally start these things in this order
– Start the daemon client in the VPS. First go back to your installed wallet directory, 
```
./ankerd
```
– From the Control wallet debug console
```
startmasternode alias false <mymnalias>
```
where <mymnalias> is the name of your masternode alias (without brackets)

The following should appear:
    “overall” : “Successfully started 1 masternodes, failed to start 0, total 1”,
    “detail” : [
    {
    “alias” : “<mymnalias>”,
    “result” : “successful”,
    “error” : “”
    }

– Back in the VPS (remote wallet), start the masternode
```
./anker-cli startmasternode local false
```
– A message “masternode successfully started” should appear

12)Use the following command to check status:
```
./anker-cli masternode status
```
You should see something like:

    {
    “txhash” : “334545645643534534324238908f36ff4456454dfffff51311”,
    “outputidx” : 0,
    “netaddr” : “45.11.111.111:51472”,
    “addr” : “D6fujc45645645445645R7TiCwexx1LA1”,
    “status” : 4,
    “message” : “Masternode successfully started”
    }

Congratulations! You have successfully created your masternode!

Now the masternode setup is complete, you are safe to remove “enablezeromint=0” from the anker.conf file of the control wallet.

Multi masternode config
=======================

The multi masternode config allows you to control multiple masternodes from a single wallet. The wallet needs to have a valid collateral output of 4000 coins for each masternode. To use this, place a file named masternode.conf in the data directory of your install:
 * Windows: %APPDATA%\Anker\
 * Mac OS: ~/Library/Application Support/Anker/
 * Unix/Linux: ~/.anker/

The new masternode.conf format consists of a space seperated text file. Each line consisting of an alias, IP address followed by port, masternode private key, collateral output transaction id, collateral output index, donation address and donation percentage (the latter two are optional and should be in format "address:percentage").

Example:
```
mn1 8.0.0.2:12365 93HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0
mn2 8.0.0.3:12365 93WaAb3htPJEV8E9aQcN23Jt97bPex7YvWfgMDTUdWJvzmrMqey aa9f1034d973377a5e733272c3d0eced1de22555ad45d6b24abadff8087948d4 0 7gnwGHt17heGpG9Crfeh4KGpYNFugPhJdh:33
mn3 8.0.0.4:12365 92Da1aYg6sbenP6uwskJgEY2XWB5LwJ7bXRqc3UPeShtHWJDjDv db478e78e3aefaa8c12d12ddd0aeace48c3b451a8b41c570d0ee375e2a02dfd9 1 7gnwGHt17heGpG9Crfeh4KGpYNFugPhJdh
```

In the example above:
* the collateral for mn1 consists of transaction 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c, output index 0 has amount 4000
* masternode 2 will donate 33% of its income
* masternode 3 will donate 100% of its income


The following new RPC commands are supported:
* list-conf: shows the parsed masternode.conf
* start-alias \<alias\>
* stop-alias \<alias\>
* start-many
* stop-many
* outputs: list available collateral output transaction ids and corresponding collateral output indexes

When using the multi masternode setup, it is advised to run the wallet with 'masternode=0' as it is not needed anymore.
