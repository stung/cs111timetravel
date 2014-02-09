sleep 4 && echo fail
sleep 3 && echo not
sleep 2 && echo i

# should run "fail \n not \n i" if there is no parallelism
# but with time travel on, it should run "i \n not \n fail"
