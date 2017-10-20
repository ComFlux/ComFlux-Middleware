Middleware for data flow integration and reconfiguration

# Dependencies 

The middleware requires json-c:

```
#!sh
sudo apt-get install libjson0 libjson0-dev

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
cmake [-DSSLENABLED:BOOL=ON] [-DKRBENABLED:BOOL=ON] [-DMQTTENABLED:BOOL=ON] .
make
sudo make install

```

The optional cmake arguments generate communication and access control modules relying on third party libraries.

| Argument  | Summary |
| :---: | ---     |
| `-DMQTTENABLED:BOOL=ON` | Enables building two com modules relying on MQTT. |
| `-DSSLENABLED:BOOL=ON` | Enables building an SSL com module and a certificate based access control module. |
| `-DKRBENABLED:BOOL=ON` | Enables building a Kerberos access control module. |


To install mosquitto:

```
#!sh
sudo apt-add-repository ppa:mosquitto-dev/mosquitto-ppa
sudo apt-get update
sudo apt-get install libmosquitto

```

To install OpensSSL:

```
#!sh
sudo apt-get install libssl-dev

```

The Kerberos module uses krb5 library linked as a submodule.


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


# Documentation

For the basic usage of ComFlux, here is a detailed tutorial: [ComFlux: Dynamic creation of pervasive applications from plug-and-play modules](tutorial.pdf)

Here is an article detailing the model and applications of ComFlux: [ComFlux: External Composition and Adaptation of Pervasive Applications](http://arxiv.org/abs/1710.06711)

We also have the following tutorial proposal: [ComFlux: Dynamic creation of pervasive applications from plug-and-play modules](tutorial-main.pdf)


