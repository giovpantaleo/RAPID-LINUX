#!/bin/bash
apt update
apt install wget -y
apt install nano -y
apt install unzip -y
apt install git -y
apt install default-jdk -y
wget https://services.gradle.org/distributions/gradle-6.4.1-bin.zip -P /tmp
unzip -d /opt/gradle /tmp/gradle-*.zip
echo 'export GRADLE_HOME=/opt/gradle/gradle-6.4.1' >> /etc/profile.d/gradle.sh 
echo 'export PATH=${GRADLE_HOME}/bin:${PATH}' >> /etc/profile.d/gradle.sh 
source /etc/profile.d/gradle.sh
mkdir /home/MECPerf
cd /home/MECPerf
git clone https://github.com/giovpantaleo/MECPerf.git
mv MECPerf/ MECPerf-master/

