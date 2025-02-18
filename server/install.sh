#!/bin/bash

sudo cp ledstrip.service /etc/systemd/system/ && \
sudo cp ledstripresume.service /etc/systemd/system/ && \
sudo systemctl daemon-reload && \
sudo systemctl enable ledstrip ledstripresume && \
sudo systemctl start ledstrip ledstripresume