<!--
	Copyright (c) 2014-2017, Emory University
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are
	permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this list of
	conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice, this list
 	of conditions and the following disclaimer in the documentation and/or other materials
	provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
	SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
	WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
	DAMAGE.

-->

###############################################################################
# Tree structure for HistomicsML docker
###############################################################################

/home/HistomicsML
│
├── hmlweb:latest
│
├── hmldb:latest
│
├── README.txt
│
└── docker-compose.yml


###############################################################################
# Install HistomicsML docker
###############################################################################

1. Install docker and docker-compose. Please refer to
  # docker install
  https://docs.docker.com/engine/installation/
	# docker-compose install
  https://docs.docker.com/compose/install/

2. Pull HistomicsML docker images (hmlweb, hmldb)
  - mkdir /home/HistomicsML
  - docker pull histomicsml/hmlweb:latest
  - docker pull histomicsml/hmldb:latest

3. Run docker
  - /home/HistomicsML$ docker-compose up -d

4. Check if two containers are correctly running
  - docker ps
  # You can see the two docker containers.

example) /home/HistomicsML$ docker ps
CONTAINER ID        IMAGE               COMMAND                  CREATED             STATUS              PORTS                                          NAMES
7864853c4e38        hmlweb:latest       "/bin/sh -c 'servi..."   2 minutes ago       Up 2 minutes        0.0.0.0:80->80/tcp, 0.0.0.0:20000->20000/tcp   docker_web_1
19cd8ef3e1ec        hmldb:latest        "docker-entrypoint..."   2 minutes ago       Up 2 minutes        0.0.0.0:3306->3306/tcp                         docker_db_1


###############################################################################
# Set up Environment
###############################################################################

1. Insert data into database
/home/HistomicsML$ docker exec -t -i docker_hmldb_1 bash
- root@19cd8ef3e1ec:/# cd /db
- root@19cd8ef3e1ec:/# ./db_run.sh


###############################################################################
# Run HistomicsML
###############################################################################

http::/localhost/HistomicsML


###############################################################################
# Miscellaneous
###############################################################################

If HistomicsML doesn't work, please set your IP Address for docker container.

Step 1. Find IP address of docker_hmldb_1 container
/home/HistomicsML$ docker inspect docker_hmldb_1 | grep IPAddress

example)
SecondaryIPAddresses": null,
            "IPAddress": "",
                    "IPAddress": "192.18.0.1",

Step 2. Modify docker_hmlweb_1 container
/home/HistomicsML$ docker exec -t -i docker_hmlweb_1 bash
root@19cd8ef3e1ec:/# cd /var/www/html/HistomicsML/db
edit accounts.php
  $dbAddress = "192.80.0.2";
  => $dbAddress = "192.80.0.1"

Step 3. Start learning server
root@19cd8ef3e1ec:/# service al_server start
