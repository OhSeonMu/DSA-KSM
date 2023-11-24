cmd_/home/osm/test_2/dsa_module/dsatest_osm.mod := printf '%s\n'   dsatest_osm.o | awk '!x[$$0]++ { print("/home/osm/test_2/dsa_module/"$$0) }' > /home/osm/test_2/dsa_module/dsatest_osm.mod
