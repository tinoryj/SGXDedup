##init
rm key -r
mkdir key
cp CA.sh key
cd key
bash CA.sh -newca
cp demoCA/cacert.pem .

##genera CA private key
#openssl genrsa -des3 -out demoCA/private/cakey.pem 1024

##genera client req file
#openssl req -new -key demoCA/private/cakey.pem -out careq.pem

##CA self sign
#openssl ca -selfsign -in careq.pem -out cacert.pem
#cp cacert.pem demoCA
#rm careq.prm

##genera client private key
openssl genrsa -des3 -out client.key 1024

##genera client public key
openssl rsa -in client.key -pubout -out clientpub.key

##genera client req file
openssl req -new -key client.key -out clientreq.pem

##genera server private key
openssl genrsa -des3 -out server.key 1024

##genera client public key
openssl rsa -in server.key -pubout -out serverpub.key

##genera client cert file
openssl req -new -key server.key -out servereq.pem

##CA sign client
openssl ca -in clientreq.pem -out clientcert.pem
rm clientreq.pem

##CA sign server
openssl ca -in servereq.pem -out servercert.pem
rm servereq.pem