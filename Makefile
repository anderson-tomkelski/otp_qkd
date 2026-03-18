.PHONY: certs clean

# NOTE: Use 'make VERBOSE=1 <target>' to show the commands being run
ifndef VERBOSE
.SILENT:
endif

.PRECIOUS: %.crt

ROOT_CRT = rootCA_auth.crt 
ROOT_KEY  = rootCA_auth.key

TMPDIR        := tmp
SERVER_TX_DIR := $(TMPDIR)/tx
SERVER_RX_DIR := $(TMPDIR)/rx
SERIAL_NUM = 1


default: certs

certs: $(SERVER_TX_DIR)/server.crt $(SERVER_RX_DIR)/server.crt alice_client1.pfx bob_client1.pfx concat

$(TMPDIR):
	mkdir -p $@

$(SERVER_TX_DIR) $(SERVER_RX_DIR): | $(TMPDIR)
	mkdir -p $@
concat:
	cat rootCA_auth.crt >> server.crt
	printf "Appended root certificate to server certificate"

%.pem: %.crt
	printf "Generating pem file '%s'...\n" $@
	cat $(basename $^).key $^ > $@
	printf "Pem file '%s' generated\n" $@

%.pfx: %.crt
	printf "Generating certificate and key bundle in pfx format, with empty password"

	openssl pkcs12 					\
		-export						\
		-inkey $(basename $@).key 	\
		-in $(basename $@).crt 		\
		-out $@ 					\
		-passout pass:


%.crt: %.csr $(ROOT_CRT)
	printf "Generating certificate '%s' with serial '%d' signed using '%s'...\n" $@ $(SERIAL_NUM) $(ROOT_CRT)

	openssl x509 -req               \
 		-in $(word 1,$^)            \
		-CA $(ROOT_CRT)            \
 		-CAkey $(ROOT_KEY)          \
 		-set_serial $(SERIAL_NUM)   \
 		-days 365                   \
 		-extfile $(basename $@).ext \
 		-out $@                     \

	printf "Certificate '%s' generated\n" $@

	$(eval SERIAL_NUM=$(shell echo $$(($(SERIAL_NUM)+1))))

%.csr: %.ext
	printf "Generating certificate signing request for '%s'...\n" $@

	openssl req                                              \
 		-newkey rsa:4096                                     \
 		-nodes                                               \
 		-days 365                                            \
 		-subj "/C=ES/O=LuxQuanta/CN=$(basename $(notdir $@))" \
 		-keyout $(basename $@).key                           \
 		-out $@                                              \


	printf "Certificate '%s' signing request generated\n" $@

%.ext:
	echo "subjectAltName = DNS:localhost,IP:127.0.0.1" >> $@

$(SERVER_TX_DIR)/server.ext: | $(SERVER_TX_DIR)
	echo "subjectAltName = DNS:localhost,IP:127.0.0.1,IP:$(SERVER_TX_HOST)" >> $@
	echo "authorityKeyIdentifier = keyid,issuer" >> $@
	echo "keyUsage = digitalSignature, keyEncipherment" >> $@
	echo "extendedKeyUsage = clientAuth,serverAuth" >> $@

$(SERVER_RX_DIR)/server.ext: | $(SERVER_RX_DIR)
	echo "subjectAltName = DNS:localhost,IP:127.0.0.1,IP:$(SERVER_RX_HOST)" >> $@
	echo "authorityKeyIdentifier = keyid,issuer" >> $@
	echo "keyUsage = digitalSignature, keyEncipherment" >> $@
	echo "extendedKeyUsage = clientAuth,serverAuth" >> $@


$(ROOT_CRT):
	printf "Generating the root certificate '%s'...\n" $(ROOT_CRT)

	openssl req -x509                                               	\
		-newkey rsa:4096												\
		-nodes                                                     		\
		-days 365                                                		\
		-subj "/CN=www.luxquanta.com" 									\
		-keyout $(ROOT_KEY)                                      		\
		-out $@                                                  		\

	printf "Root certificate '%s' generated\n" $(ROOT_CRT)

clean:
	rm -f *.crt \
		  *.csr \
		  *.pem \
	 	  *.ext \
		  *.key \
		  *.pfx

	printf "Certificates and output log removed\n"

