FROM dyn_typer:38

COPY . /
WORKDIR /flask
RUN pip install -r requirements/dev.txt
RUN pip install -r requirements/tests.txt
RUN pip install -r requirements/tests-pallets-min.txt
RUN PYTHONPATH=src pytest --cov=src
