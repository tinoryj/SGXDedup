# Accelerating Encrypted Deduplication via SGX

## Introduction

Encrypted deduplication preserves the deduplication effectiveness on encrypted data and is attractive for outsourced storage.  However, existing encrypted deduplication approaches build on expensive cryptographic primitives that incur substantial performance slowdown.  We present SGXDedup, which leverages Intel SGX to speed up encrypted deduplication based on server-aided message-locked encryption (MLE), while preserving security via SGX.  SGXDedup implements a suite of secure interfaces to execute MLE key generation and proof-of-ownership operations in SGX enclaves.  It also proposes various designs to support secure and efficient enclave operations.  Evaluation on synthetic and real-world workloads shows that SGXDedup achieves significant speedups and maintains high bandwidth and storage savings.

## Publication

## Prerequisites

## Installation Guide

## Usage

### Uasge Samples