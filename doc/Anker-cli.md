Anker Core RPC command list
=====================

### Anker 
- checkbudgets
- createmasternodebroadcast "command" ( "alias")
- createmasternodekey
- decodemasternodebroadcast "hexstring"
- getbudgetinfo ( "proposal" )
- getbudgetprojection
- getbudgetvotes "proposal-name"
- getmasternodecount
- getmasternodeoutputs
- getmasternodescores ( blocks )
- getmasternodestatus
- getmasternodewinners ( blocks "filter" )
- getnextsuperblock
- getpoolinfo
- listcoldstakers
- listmasternodeconf ( "filter" )
- listmasternodes ( "filter" )
- makekeypair [prefix]
- masternode "command"...
- masternodeconnect "address"
- masternodecurrent
- masternodedebug
- mnbudget "command"... ( "passphrase" )
- mnbudgetrawvote "masternode-tx-hash" masternode-tx-index "proposal-hash" yes|no time "vote-sig"
- mnbudgetvote "local|many|alias" "votehash" "yes|no" ( "alias" )
- mnfinalbudget "command"... ( "passphrase" )
- mnsync "status|reset"
- preparebudget "proposal-name" "url" payment-count block-start "anker-address" monthly-payment
- relaymasternodebroadcast "hexstring"
- setupcoldstaking
- setupmasternode "node" "local|remote"
- spork <name> [<value>]
- startcoldstaking
- startmasternode "local|all|many|missing|disabled|alias" lockwallet ( "alias" )
- submitbudget "proposal-name" "url" payment-count block-start "anker-address" monthly-payment "fee-tx"

### Blockchain 
- findserial "serial"
- getbestblockhash
- getblock "hash" ( verbose )
- getblockchaininfo
- getblockcount
- getblockhash index
- getblockheader "hash" ( verbose )
- getchaintips
- getdifficulty
- getmempoolinfo
- getrawmempool ( verbose )
- gettxout "txid" n ( includemempool )
- gettxoutsetinfo
- verifychain ( numblocks )

### Control 
- getinfo
- help ( "command" )
- stop

### Generating 
- getgenerate
- gethashespersec
- setgenerate generate ( genproclimit )

### Mining 
- getblocktemplate ( "jsonrequestobject" )
- getmininginfo
- getnetworkhashps ( blocks height )
- prioritisetransaction <txid> <priority delta> <fee delta>
- reservebalance ( reserve amount )
- submitblock "hexdata" ( "jsonparametersobject" )

### Network 
- addnode "node" "add|remove|onetry"
- clearbanned
- disconnectnode "node"
- getaddednodeinfo dns ( "node" )
- getconnectioncount
- getnettotals
- getnetworkinfo
- getpeerinfo
- listbanned
- ping
- setban "ip(/netmask)" "add|remove" (bantime) (absolute)

### Rawtransactions 
- createrawtransaction [{"txid":"id","vout":n},...] {"address":amount,...}
- decoderawtransaction "hexstring"
- decodescript "hex"
- getrawtransaction "txid" ( verbose )
- searchrawtransactions <address> [verbose=1] [skip=0] [count=100]
- sendrawtransaction "hexstring" ( allowhighfees )
- signrawtransaction "hexstring" ( [{"txid":"id","vout":n,"scriptPubKey":"hex","redeemScript":"hex"},...] ["privatekey1",...] sighashtype )

### Util 
- createmultisig nrequired ["key",...]
- createwitnessaddress "script"
- estimatefee nblocks
- estimatepriority nblocks
- validateaddress "bitcoinaddress"
- verifymessage "ankeraddress" "signature" "message"

### Wallet 
- addmultisigaddress nrequired ["key",...] ( "account" )
- addwitnessaddress "address" ( p2sh )
- autocombinerewards true|false ( threshold )
- backupwallet "destination"
- bip38decrypt "ankeraddress"
- bip38encrypt "ankeraddress"
- dumpallprivatekeys "filename"
- dumpprivkey "ankeraddress"
- dumpwallet "filename"
- encryptwallet "passphrase"
- getaccount "ankeraddress"
- getaccountaddress "account"
- getaddressesbyaccount "account"
- getbalance ( "account" minconf includeWatchonly )
- getnewaddress ( "account" "address_type" )
- getrawchangeaddress ( "address_type" )
- getreceivedbyaccount "account" ( minconf )
- getreceivedbyaddress "ankeraddress" ( minconf )
- getstakesplitthreshold
- getstakingstatus
- gettransaction "txid" ( includeWatchonly )
- getunconfirmedbalance
- getwalletinfo
- importaddress "address" ( "label" rescan p2sh )
- importprivkey "ankerprivkey" ( "label" rescan )
- importpubkey "pubkey" ( "label" rescan )
- importwallet "filename"
- keypoolrefill ( newsize )
- listaccounts ( minconf includeWatchonly)
- listaddressgroupings
- listlockunspent
- listreceivedbyaccount ( minconf includeempty includeWatchonly)
- listreceivedbyaddress ( minconf includeempty includeWatchonly)
- listsinceblock ( "blockhash" target-confirmations includeWatchonly)
- listtransactions ( "account" count from includeWatchonly)
- listunspent ( minconf maxconf  ["address",...] )
- lockunspent unlock [{"txid":"txid","vout":n},...]
- move "fromaccount" "toaccount" amount ( minconf "comment" )
- multisend <command>
- sendfrom "fromaccount" "toankeraddress" amount ( minconf "comment" "comment-to" )
- sendmany "fromaccount" {"address":amount,...} ( minconf "comment" )
- sendtoaddress "ankeraddress" amount ( "comment" "comment-to" )
- sendtoaddressix "ankeraddress" amount ( "comment" "comment-to" )
- setaccount "ankeraddress" "account"
- setstakesplitthreshold value
- settxfee amount
- signmessage "ankeraddress" "message"

### Zerocoin 
- exportzerocoins include_spent ( denomination )
- getarchivedzerocoin
- getspentzerocoinamount hexstring index
- getzerocoinbalance
- getzphrseed
- importzerocoins importdata
- listmintedzerocoins
- listspentzerocoins
- listzerocoinamounts
- mintzerocoin <amount>
- reconsiderzerocoins
- resetmintzerocoin
- resetspentzerocoin
- setzphrseed "seed"
- spendzerocoin <amount> <mintchange [true|false]> <minimizechange [true|false]>  <securitylevel [1-100]> <address>
