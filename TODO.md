## Beta version:

### file_list.json:
* add version field
* add file size field
* add optional downloadlink field

### update-client:
* create shared memory and API to interact with browser
* check if file is not fully downloaded and continue downloading
* HTTPS: validate SSL cert

### update-server:
* autogenerate file_list.json
* add possibility to limit bandwidth
* limit connections by ``maxClients`` in config

### file-list-generator:
* create file_list.json by folder as an argument
