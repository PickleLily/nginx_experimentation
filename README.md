Project Description:
    We are taking falco output and ...

How to Run:

	- open /etc/falco and edit the configuration file to change what variabled we are recording via falco
		- edit the values.yaml file to capture more or less data from the falco script...

Generating an output file:

    dot -Tpng <graphName>.dot -o <outputName>.png


** Each time you run falco you must reset it to fix the issues with dev/falco0:

    sudo falcoctl driver config --type kmod
    sudo falcoctl driver install
