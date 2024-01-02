# RunTyper

---

0. Before run, a mysql server should be started. The related infornation need to be filled in the ./Dockerfile. The db must not be built before running.

1. Build the docker for dynamic type inference.
```
docker build -t dyn_typer:38 .
```

2. Then, we use the flask as an example to show how the dynamic type information is collected. Just use the docker previously built to deploy and run the project to be test.
    
```
cd benchmark/test_0
docker build -t project_test:flask .
```

3. The type information is stored in the database and now can be querried.
