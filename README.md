Middleware for dynamic flow reconfiguration

# Dependencies 

The middleware requires json-c:

```
#!sh
sudo apt-get install libjson0 libjson0-dev

```

Some com and access control modules require specific libraries. To install mosquitto:

```
#!sh
sudo apt-add-repository ppa:mosquitto-dev/mosquitto-ppa
sudo apt-get update
sudo apt-get install libmosquitto

```

To install openssl:

```
#!sh
sudo apt-get install libssl-dev

```

# Installation

Download the git repository:

```
#!sh
git clone https://github.com/ComFlux/ComFlux-Middleware.git
```
 
The are included as submodules. The  added submodules are: uthash, kerberros, and jsonschema-c. 

```
#!sh
cd ComFlux-Middleware
git submodule init
git submodule update --recursive

```

Install the middleware:

```
#!sh
cmake .
make
sudo make install

```

Note that to generate SSL libraries and Kerberos access module the following command must replace the first one:

```
#!sh
cmake -DSSLENABLED:BOOL=ON -DKRBENABLED:BOOL=ON .

```

To generate mosquitto com modules set the MQTTENABLED argument

```
#!sh
cmake -DMQTTENABLED:BOOL=ON .

```

# API structure
Through the API the following headers are available:

| File  | Summary |
| :---: | ---     |
| **middleware.h** | Communication and configuration functionality with the middleware core. |
| **endpoint.h** | Endpoint definitions and endpoint specific commands. |
| **load_mw_config.h** | To facilitate middleware deployment configuration can be applied from json structures or fles. |


# Examples

### Simple source - sink
Run a simple source - sink communication by opening two terminals and typing

```
#!sh
cd build/bin/examples
./simple_source src_mw_cfg.json

```

```
#!sh
cd build/bin/examples
./simple_sink 127.0.0.1:1503 snk_mw_cfg.json

```

### Resource discovery

Map the sink to the source by querying a Resource Discovery Component for the source. First, run an RDC
```
#!sh
rdc

```

Then start `lookup_source` and `lookup_sink`.
```
#!sh
cd build/bin/examples
./lookup_source src_mw_cfg.json

```

```
#!sh
cd build/bin/examples
./lookup_sink snk_mw_cfg.json "\"ep_name\" = \"ep_source\"

```

### Dynamic connection management and reconfiguration

Watch this space

# Compile your new component
The distribution provides the core and the library of the middleware. To compile your component run:

```
#!sh
 cc -o your_component your_component.c -lmiddleware_api -lmiddleware_utils

```

All functions presented so far are implemented in the `middleware_api`. The middleware_utils library contains data structures used in the mw and functions to manipulate them. For instance json, hashmap, array, message.
