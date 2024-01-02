FROM ubuntu:20.04

RUN apt update
RUN apt install -y ca-certificates

ENV DYN_TYPER_SQL_HOST=YOUR_SQL_HOST
ENV DYN_TYPER_SQL_UNAME = YOUR_SQL_USER_NAME
ENV DYN_TYPER_SQL_PASSWD = YOUR_SQL_PASSWORD
ENV DYN_TYPER_SQL_DB = YOUR_SQL_DB_NAME
ENV DYN_TYPER_SQL_PORT = YOUR_SQL_PORT

RUN echo "deb https://mirror.sjtu.edu.cn/ubuntu/ focal main restricted universe multiverse\ndeb https://mirror.sjtu.edu.cn/ubuntu/ focal-updates main restricted universe multiverse\n# deb-src https://mirror.sjtu.edu.cn/ubuntu/ focal-updates main restricted universe multiverse\ndeb https://mirror.sjtu.edu.cn/ubuntu/ focal-backports main restricted universe multiverse\n# deb-src https://mirror.sjtu.edu.cn/ubuntu/ focal-backports main restricted universe multiverse\ndeb https://mirror.sjtu.edu.cn/ubuntu/ focal-security main restricted universe multiverse\n# deb-src https://mirror.sjtu.edu.cn/ubuntu/ focal-security main restricted universe multiverse\n# deb https://mirror.sjtu.edu.cn/ubuntu/ focal-proposed main restricted universe multiverse\n# deb-src https://mirror.sjtu.edu.cn/ubuntu/ focal-proposed main restricted universe multiverse\n" > /etc/apt/sources.list

RUN echo "export LC_ALL=C.UTF-8\nexport LANG=C.UTF-8" >> ~/.bashrc
RUN echo "abc"
RUN apt update
RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata
RUN apt install -y apt-utils
RUN apt install rustc cargo build-essential libssl-dev zlib1g-dev libncurses5-dev \
  libncursesw5-dev libreadline-dev libsqlite3-dev libgdbm-dev \
  libdb5.3-dev libbz2-dev libexpat1-dev liblzma-dev libffi-dev git watchdog gcc g++ libc6 libc6-dev -y
RUN apt install libmariadb-dev libmariadbd-dev libmariadbclient-dev vim -y

COPY cpython /cpython
WORKDIR /cpython
RUN LDFLAGS=-L/usr/lib/x86_64-linux-gnu/ LIBS=-lmariadb ./configure --with-ensurepip=install --with-pydebug
# --bindir=/usr/bin --sbindir=/usr/sbin --sysconfdir=/etc --libdir=/usr/lib --localstatedir=/var --includedir=/usr/include
RUN make -j; make install -j
RUN ln -s /usr/local/bin/python3.8 /usr/local/bin/python
RUN ln -s /usr/local/bin/pip3.8 /usr/local/bin/pip
RUN pip3 config set global.index-url https://mirror.sjtu.edu.cn/pypi/web/simple
# COPY pip.ini /etc/pip.conf 
RUN pip3 install pip==20.0.2
RUN pip3 install pytest pytest-coverage poetry tox pytest-timeout
WORKDIR /