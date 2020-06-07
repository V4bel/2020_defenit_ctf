FROM ubuntu:18.04

MAINTAINER V4bel yhajug0012@gmail.com

RUN apt update
RUN apt-get install -y xinetd gcc-multilib qemu-kvm
RUN apt-get install -y curl
RUN apt-get install -y cpio
RUN apt-get install -y python3 python3-pip
ENV TERM=linux

RUN useradd visitor
COPY files/ /chall
COPY xinetd /etc/xinetd.d/visitor
RUN /etc/init.d/xinetd restart
WORKDIR /chall
#ADD /etc/xinetd.d

RUN chmod 777 /chall
RUN chmod 460 /chall/*
RUN chown visitor:root /chall/*
RUN chmod +x /chall/start.sh
RUN chmod +x /chall/pow_check.py

RUN echo "visitor 3333/tcp" >> /etc/services
EXPOSE 3333

CMD ["/usr/sbin/xinetd","-dontfork"]
