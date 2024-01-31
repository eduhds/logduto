#!/bin/bash

# First, start logduto in a separate terminal
# ./build/debug/bin/logduto https://jsonplaceholder.typicode.com

methods=(GET POST PUT PATCH DELETE)

for i in {1..10}; do
  for method in "${methods[@]}"
  do
    printf "%s\n" $method

    if [ "$method" == "POST" ]
    then
      curl -X $method http://localhost:8099/posts \
        -H "Content-Type: application/json" \
        -d '{"title": "foo", "body": "bar", "userId": 1}'
    elif [ "$method" == "PUT" ]
    then
      curl -X $method http://localhost:8099/posts/1 \
        -H "Content-Type: application/json" \
        -d '{"title": "foo", "body": "bar", "userId": 1}'
    elif [ "$method" == "PATCH" ]
    then
      curl -X $method http://localhost:8099/posts/1 \
        -H "Content-Type: application/json" \
        -d '{"title": "foo"}'
    else
      curl -X $method http://localhost:8099/posts/1
    fi

    printf "\n"
    sleep 0.5
  done
  sleep 2
done