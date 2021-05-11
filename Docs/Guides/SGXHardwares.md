# SGX-hardware list
This is a list of hardware which supports Intel SGX - Software Guard Extensions. 

[![Build Status](https://travis-ci.org/ayeks/SGX-hardware.svg?branch=master)](https://travis-ci.org/ayeks/SGX-hardware)

## Desktop

The CPU and the motherboard BIOS must support SGX. SGX is turned off by default and must be enabled via MSR.IA32\_Feature\_Control.SGX\_Enable. Only the BIOS can make changes to the IA32\_Feature\_Control.

### Desktop CPUs affected by the product change notification from 2015

Use the [ark.intel.com](https://ark.intel.com/Search/FeatureFilter?productType=processors&SoftwareGuardExtensions=true) database to list all Intel CPUs that have the SGX feature.

Be cautious with the following 2015 i7,i5 and E3 CPUs. According to the [Product Change Notification](http://qdms.intel.com/dm/i.aspx/5A160770-FC47-47A0-BF8A-062540456F0A/PCN114074-00.pdf) from October 1, 2015. Only the listed CPUs with the following S-Spec Numbers are able to execute SGX instructions. For all other CPUs the Intel CPU database should be correct.

| Marketing Name | Processor# | Post-Conversion S-Spec |
|----------------|------------|------------------------|
| Intel® Core    | i7-6700K   | SR2L0     |
| Intel® Core    | i5-6400T   | SR2L1     |
| Intel® Core    | i7-6700    | SR2L2     |
| Intel® Core    | i7-6700T   | SR2L3     |
| Intel® Core    | i5-6600K   | SR2L4     |
| Intel® Core    | i5-6600    | SR2L5     |
| Intel® Core    | i5-6500    | SR2L6     |
| Intel® Core    | i5-6400    | SR2L7     |
| Intel® Core    | i5-6500T   | SR2L8     |
| Intel® Core    | i5-6600T   | SR2L9     |
| Intel® Xeon®   | E3-1280 v5    | SR2LC  |
| Intel® Xeon®   | E3-1240 v5    | SR2LD  |
| Intel® Xeon®   | E3-1230 v5    | SR2LE  |
| Intel® Xeon®   | E3-1270 v5    | SR2LF  |
| Intel® Xeon®   | E3-1220 v5    | SR2LG  |
| Intel® Xeon®   | E3-1260L v5   | SR2LH  |
| Intel® Xeon®   | E3-1225 v5    | SR2LJ  |
| Intel® Xeon®   | E3-1275 v5    | SR2LK  |
| Intel® Xeon®   | E3-1245 v5    | SR2LL  |
| Intel® Xeon®   | E3-1235L v5   | SR2LM  |
| Intel® Xeon®   | E3-1240L v5   | SR2LN  |

Checkout [Intel ARK specifications](https://ark.intel.com/content/www/us/en/ark.html) for newer CPU models wit hSGX support.

### CPUs without Platform Service Enclave functionality

As per the last comment on this [thread](https://software.intel.com/en-us/forums/intel-software-guard-extensions-intel-sgx/topic/737881), Intel Xeon E3 processors as of today (Jul. 2017) do not have an Intel Management Engine. Therefore, the kernel will never be able to expose the device `/dev/mei0`.  What this means in terms of SGX is that Trusted Platform Service Functions (monotonic counters, trusted time) are not available on Xeon E3.

The following sgx_tservice functions are not available:
```
    sgx_create_pse_session
    sgx_close_pse_session
    sgx_get_ps_sec_prop
    sgx_get_trusted_time
    sgx_create_monotonic_counter_ex
    sgx_create_monotonic_counter
    sgx_destroy_monotonic_counter
    sgx_increment_monotonic_counter
    sgx_read_monotonic_counter
```
@lacabra discussed a workaround in [issue 43](https://github.com/ayeks/SGX-hardware/issues/43):

> ... And while not ideal, you can circumvent these, so you can arguably have fully capable SGX instances: you should be able to open a TLS connection to an NTP server you trust from within the enclave to obtain a source of trusted time. If you think of trusted monotonic counter as an instance of trusted time, you could get both using the same mechanism (these are suggestions from an Intel SGX architect).


### Desktop Mainboards

| Vendor | Model | Driver Type | Version | Source | Release Date |
|--------|-------|-------------|---------|--------|--------------|
| ASRock | E3V5 WS | BIOS | 7.10 | [see Issue 13](https://github.com/ayeks/SGX-hardware/issues/13) | 03 Feb 2017 |
| ASRock | Z270 Extreme 4 | - | - | [see Issue 15](https://github.com/ayeks/SGX-hardware/issues/15) | 2017 |
| ASRock | Z370 Gaming ITX | BIOS | 1.4 | [see Issue 36](https://github.com/ayeks/SGX-hardware/issues/36) | 2017 |
| MSI | H170 Gaming M3 Board | BIOS | - | [see Issue 21](https://github.com/ayeks/SGX-hardware/issues/21) | 4 May 2017 |
| Gigabyte | GA-Z270MX-Gaming 5 | BIOS | F3 | [see Issue 20](https://github.com/ayeks/SGX-hardware/issues/20) | 4 May 2017 |
| Gigabyte | Z370 AORUS Ultra Gaming (rev. 1.0) | BIOS | - | [see Issue 33](https://github.com/ayeks/SGX-hardware/issues/20) | 30 Nov 2017 |
| Gigabyte | Z370 AORUS Ultra Gaming (rev. 2.0) | BIOS | F4 | [see Issue 53](https://github.com/ayeks/SGX-hardware/issues/53) | 1 Oct 2018 |
| MAXSUN | MS-EARTHSHAKER H310M-VH | - | - | [see Issue 54 about the outdated BIOS!](https://github.com/ayeks/SGX-hardware/issues/54) | 10 Oct 2018 |
| MSI | Z370-A PRO | Bios update required | - | [see Issue 56](https://github.com/ayeks/SGX-hardware/issues/56) | 28 Dec 2018 |
| ASRock | Z390 Pro 4 | BIOS | 4.3 | [see Issue 64](https://github.com/ayeks/SGX-hardware/issues/64) | 16 Sep 2019 |

## Notebooks 

The following devices should support SGX according to available drivers. Check the detailed information about the supported models on the individual driver sites.

| Vendor | Model | Driver Type | Version | Supported OS | Source | Release Date |
|--------------|-------|-------------|---------|--------------|--------|----|
| Dell  | Inspiron 11 i3153, Inspiron 11 i3158, Inspiron 13 i7353, Inspiron 13 i7359, Inspiron 15 i7568 | Chipset Driver   | 1.0.26920. 1393, A00  | Windows 8.1 64-bit, Windows 10 64-bit | dell.com | 10 Sep 2015 |
| Dell | Inspiron 15 7000 Series 7559 | BIOS | 1.0.3 | Windows/DOS | [dell.com](https://www.dell.com/support/home/de-de/drivers/DriversDetails?driverId=3FWTM) | 09 Oct 2015 |
| HP | HP 240 G4 Notebook PC, HP 246 G4 Notebook PC, HP Notebook 14g, HP Notebook 14q  | Chipset | 1.0.26448. 1348 REV: A PASS: 5 | Microsoft Windows 10 64 | [ftp.hp.com](http://ftp.hp.com/pub/softpaq/sp72001-72500/sp72357.html) | 07 Aug 2015 |
| HP | ENVY 15 Notebook PC | Chipset | 1.0.26805. 1389 REV: A PASS: 4 | Windows 10 64bit, Windows 7 Prof. | [ftp.hp.com](http://ftp.hp.com/pub/softpaq/sp72001-72500/sp72491.html) | 17 Aug 2015 |
| HP | HP ENVY x360 Convertible | Chipset | 1.0.26805.1389 REV: A PASS: 5 | Windows 10 64 | [ftp.hp.com](http://ftp.hp.com/pub/softpaq/sp72001-72500/sp72316.html) | 04 Sep 2015 |
| HP | HP Spectre x2 Detachable Ultrabook PC | Chipset | 1.0.26805. 1389 REV: A PASS: 9 | Windows 10 64 | [ftp.hp.com](http://ftp.hp.com/pub/softpaq/sp72501-73000/sp72989.html) | 02 Oct 2015 |

## Hardware with out-of-the-box SGX support

Newer devices have out of the box SGX BIOS support. Sometimes it is mentioned in their documentation. If you have a newer device and see SGX in the BIOS, message me so I can add the device to the list.
If you need a SGX capable server have a look at the [Intel SGX server block](https://www-ssl.intel.com/content/www/us/en/data-center-blocks/business/sgx-server-blocks.html). It consists of a complete server with Xeon processor, ram, mainboard and drives. Driver issues should't be a problem.

| Device | Vendor | Model |  Source | Date | Confirmed |
|--------|--------|-------|---------|------|-----------|
| Laptop | Lenovo  | ThinkPad P70, P50S, T560, T460p, T460s, T460, X260, X230  | [see Issue 7](https://github.com/ayeks/SGX-hardware/issues/7) | 22 May 2016 | P50, T460s, [X260](http://pastebin.com/c4SP3mNr), [X230](https://github.com/ayeks/SGX-hardware/issues/29) |
| Laptop | Lenovo | YOGA 720, Carbon X1 | [see Issue 49](https://github.com/ayeks/SGX-hardware/issues/49) | 2 Jul 2018 | YOGA 720, Carbon X1 |
| Laptop | Lenovo | Ideapad Yoga C940 | [see issue 77](https://github.com/ayeks/SGX-hardware/issues/77) | 13 Mar 2021 | Ideapad Yoga C940 |
| PC | Dell  | newer OptiPlex, Latitude, Vostro, Precision  | [see Issue 5](https://github.com/ayeks/SGX-hardware/issues/5) | 20 Apr 2015 | Precision 7710 |
| Desktop PC | Dell  | Optiplex 5040 | dell.com technical-spec-sheet.pdf | 3 Dec 2015 | |
| Laptop | Dell | XPS 13 |  [see Issue 12](https://github.com/ayeks/SGX-hardware/issues/12), [see Issue 16](https://github.com/ayeks/SGX-hardware/issues/16), [Issue 69](https://github.com/ayeks/SGX-hardware/issues/69), [Issue 75](https://github.com/ayeks/SGX-hardware/issues/75) | 20 Jan 2017 | XPS 13 9560, 9360, [9380](https://github.com/ayeks/SGX-hardware/issues/75) |
| Laptop | Dell | Alienware 13 R3  | mail | 30 Jan 2017 | Alienware 13 R3 (Kaby Lake i7-7700HQ) |
| Laptop | Dell | Alienware 15 R3  | [see PR 30](https://github.com/ayeks/SGX-hardware/pull/30) | 05 Sep 2017 | Alienware 15 R3 (Skylake i7-6820HK) |
| Laptop | Dell | Inspiron 5378, 5578, 7378, 7579, 7779 | [see Issue 18](https://github.com/ayeks/SGX-hardware/issues/18) | 18 Apr 2017 | Inspiron 15 5578 2-in-1 with Intel Kabylake 7500U |
| Laptop | Dell | Latitude 7280 | [see Issue 52](https://github.com/ayeks/SGX-hardware/issues/52) | 26 Sep 2018 | Dell Latitude 7280 |
| Mini PC | Intel NUC Kit | NUC6i3SYK, NUC6i7KYK, NUC6i5SYK, NUC6i3SYH, NUC6i5SYH, NUC7i3BNH | SGX software for NUC | 30 Jun 2016 | [NUC7i3BNH](https://github.com/ayeks/SGX-hardware/issues/22) |
| Compute Stick | Intel Compute Stick | [STK2m364CC](https://ark.intel.com/products/91981/Intel-Compute-Stick-STK2m364CC) | [see Issue 31](https://github.com/ayeks/SGX-hardware/issues/31) | 12 Nov 2017 | STK2m364CC |
| Laptop | HP  | Pavilion 15-ab251ur  | [see Issue 14](https://github.com/ayeks/SGX-hardware/issues/14) | 18 Feb 2017 | HP Pavilion 15-ab251ur, CPU i7-6500U |
| Laptop | ASUS | X456UA | [see Issue 18](https://github.com/ayeks/SGX-hardware/issues/18) | 18 Apr 2017 |  |
| Workstation | HP  | Z2 Mini G3  | [see Pull request 19](https://github.com/ayeks/SGX-hardware/pull/19) | 21 Apr 2017 | HP Z2 Mini G3, CPU Xeon E3-1245 v5 |
| Workstation | HP  | HP Z240 Tower Workstation  | [see Pull request 23](https://github.com/ayeks/SGX-hardware/pull/23) | 16 Mar 2017 | HP Z240 Tower Workstation, Intel(R) Xeon(R) CPU E3-1240 v5 @ 3.50GHz|
| Server | Supermicro | system [5019-MR](http://www.supermicro.com/products/system/1U/5019/SYS-5019S-MR.cfm), mainboard [X11SSH-F BIOS 1.0b or 2.0b](http://www.supermicro.com/products/motherboard/Xeon/C236_C232/X11SSH-F.cfm) | [Pressrelease](http://www.supermicro.com/newsroom/pressreleases/2015/press150901_Embedded_IoT_Skylake.cfm) [see Issue 6](https://github.com/ayeks/SGX-hardware/issues/6) | 19 May 2016| [platform services only on specific ME versions](https://github.com/ayeks/SGX-hardware/issues/24) |
| Server | Dell | PowerEdge R230, BIOS [2.4.3](https://www.dell.com/support/home/de-de/drivers/driversdetails?driverId=WDY2P) or later | mail | 23 Feb 2018 | Dell R230, CPU Xeon 1270 v6, BIOS v2.4.3 and BIOS v2.5.0 |
| Server | Dell | PowerEdge R340, BIOS 2.2.3 | [see Issue 70](https://github.com/ayeks/SGX-hardware/issues/70) | 16 May 2020 | Dell PowerEdge R340 |
| Mini PC | Intel NUC Kit | NUC7i5BNH | [see Issue 67](https://github.com/ayeks/SGX-hardware/issues/67) | 9 Oct 2019 | NUC7i5BNH with I5-7260U |
| Server | Dell | PowerEdge R330 | Simply Staking's Testing (only works baremetal) | Dec 2020 | Dell PowerEdge R330 |
| Server | Dell | PowerEdge R340 | Simply Staking's Testing (works baremetal and with vSphere 7 vSGX) | Dec 2020 | Dell PowerEdge R340 (Intel Xeon E-2288G) |

## Hardware with SGX2 support

It seems that Intel [Gemini Lake](https://ark.intel.com/content/www/us/en/ark/products/codename/83915/gemini-lake.html) processors support SGX2. We will list the confirmed devices here as soon as they get reported.

| Device | Vendor | Model |  Source | Date | Confirmed |
|--------|--------|-------|---------|------|-----------|
| Mini PC | Intel NUC Kit | NUC7CJYH, NUC7PJYH | [Issue 48](https://github.com/ayeks/SGX-hardware/issues/48), [Pull Request 68](https://github.com/ayeks/SGX-hardware/pull/68) | 4 Apr 2019 | [NUC7CJYH](https://github.com/ayeks/SGX-hardware/issues/48), [NUC7PJYH](https://github.com/ayeks/SGX-hardware/pull/68) |
| Laptop | Dell | XPS 13 9300 | [Issue 75](https://github.com/ayeks/SGX-hardware/issues/75) | 24 Feb 2021 | [XPS 13 9300](https://github.com/ayeks/SGX-hardware/issues/75) |
| Laptop | Lenovo |  Ideapad Yoga C940 | [Issue 77](https://github.com/ayeks/SGX-hardware/issues/77) | 13 Mar 2021 | [Ideapad Yoga C940](https://github.com/ayeks/SGX-hardware/issues/77) |

## Cloud Vendors

Packet.net, IBM and Alibaba have Bare Metal Instances available that allow the execution of SGX instructions to create secure enclaves in the cloud. See [IBM Cloud Bare Metal Instances](https://www.ibm.com/blogs/bluemix/2018/05/data-use-protection-ibm-cloud-using-intel-sgx/) and [Alibaba ECS Bare Metal Instance](https://www.alibabacloud.com/product/ebm). If you were able to experiment with SGX in the Cloud, please let us know!

Both [AWS](https://github.com/ayeks/SGX-hardware/issues/37) and [Google](https://github.com/ayeks/SGX-hardware/issues/38) have CPUs that capable of SGX but the execution is disabled. We discussed that into detail [here](https://github.com/ayeks/SGX-hardware/issues/37). According to a [Intel forum post](https://software.intel.com/en-us/forums/intel-software-guard-extensions-intel-sgx/topic/755636).

| Vendor | Servicename | CPU SGX capable | SGX activated in BIOS | Date | Source |
|--------|-------------|-----------------|-----------------------|------|-----------|
| AWS | EC2 C5 instances | YES, SGX1 and SGX2 | NO, SGX not activated in BIOS | Apr 2018 | [Issue 37](https://github.com/ayeks/SGX-hardware/issues/37) |
| Azure | [Azure Confidential Computing Public Preview](https://azuremarketplace.microsoft.com/en-us/marketplace/apps/microsoft-azure-compute.confidentialcompute?tab=Overview) | YES, SGX1 | YES, SGX1 | Oct 2018 | [Blog](https://azure.microsoft.com/en-us/blog/protect-data-in-use-with-the-public-preview-of-azure-confidential-computing/) |
| Google | N1 instances | YES, SGX1 and SGX2 | NO, SGX not activated in BIOS | Apr 2018 | [Issue 38](https://github.com/ayeks/SGX-hardware/issues/38) |
| IBM | [IBM Cloud Bare Metal Instances](https://www.ibm.com/blogs/bluemix/2018/05/data-use-protection-ibm-cloud-using-intel-sgx/) | YES, unknown version | YES | May 2018 | [Issue 46](https://github.com/ayeks/SGX-hardware/issues/46) |
| packet.net | [Reserved Hardware](https://www.packet.com/cloud/all-features/) | YES, SGX1 | YES | Apr 2018 | [Issue 44](https://github.com/ayeks/SGX-hardware/issues/44) |
| Alibaba Cloud | [ECS Bare Metal Instance](https://www.alibabacloud.com/product/ebm) | YES, unknown version | YES | Sep 2018 | [Docs](https://www.alibabacloud.com/help/doc-detail/60576.html), [Issue 50](https://github.com/ayeks/SGX-hardware/issues/50) |
| OVHcloud  | [Infrastructure Dedicated Servers](https://www.ovh.com/ca/en/dedicated-servers/infra/) | YES, unknown version | YES | Sep 2019 | [Docs](https://www.ovh.ie/dedicated_servers/software-guard-extensions/), [Issue 66](https://github.com/ayeks/SGX-hardware/issues/66) |

## Device with a SGX CPU, but no BIOS support

The following devices have a SGX capable CPU included, but don't have the required BIOS settings. So you cannot execute SGX instructions on those devices. **Do NOT buy for SGX development:**

| Device | Vendor | Model |  Source | Date |
|--------|--------|-------|---------|------|
| Laptop | Apple  | MacBook Pro (15-inch, 2017) | [Issue 26](https://github.com/ayeks/SGX-hardware/issues/26) | 24 Jul 2017 |
| Laptop | Apple  | MacBook Pro (15-inch, 2018) | [Issue 57](https://github.com/ayeks/SGX-hardware/issues/57) | 15 Feb 2019 |
| Laptop | Apple  | MacBook Pro (13-inch, 2018) | [Issue 58](https://github.com/ayeks/SGX-hardware/issues/58) | 25 Feb 2019 |
| Laptop | Microsoft | Surface Book 2 | [Issue 52](https://github.com/ayeks/SGX-hardware/issues/52) | 26 Sep 2018 |
| Laptop | Razer | Blade (2016) | [Issue 60](https://github.com/ayeks/SGX-hardware/issues/60) | 6 Jul 2019 |

## Test SGX

You can check if SGX is enabled on you system with the test_sgx.c. Just compile and run it:
```
$ gcc test-sgx.c -o test-sgx
$ ./test-sgx
```

See [Issue 17](https://github.com/ayeks/SGX-hardware/issues/17) for the execution in Visual Studio.

### SGX is available for your CPU but not enabled in BIOS
eg. on [2017 MacBook Pro's](https://github.com/ayeks/SGX-hardware/issues/26)
```
...
Extended feature bits (EAX=07H, ECX=0H)
eax: 0 ebx: 29c6fbf ecx: 0 edx: 0
sgx available: 1

CPUID Leaf 12H, Sub-Leaf 0 of Intel SGX Capabilities (EAX=12H,ECX=0)
eax: 0 ebx: 0 ecx: 0 edx: 0
sgx 1 supported: 0
sgx 2 supported: 0
MaxEnclaveSize_Not64: 0
MaxEnclaveSize_64: 0
...
```

### CPU SGX functions are deactivated or SGX is not supported
Eg. on [public clouds](https://github.com/ayeks/SGX-hardware/issues/37) or [2015 MacBooks Pro's](https://github.com/ayeks/SGX-hardware/issues/25))

```
...
Extended feature bits (EAX=07H, ECX=0H)
eax: 0 ebx: d19f4fbb ecx: 8 edx: 0
sgx available: 0

CPUID Leaf 12H, Sub-Leaf 0 of Intel SGX Capabilities (EAX=12H,ECX=0)
eax: 2ff ebx: a80 ecx: a88 edx: 0
sgx 1 supported: 1
sgx 2 supported: 1
MaxEnclaveSize_Not64: 0
MaxEnclaveSize_64: 0
...
```

### SGX is available for your CPU and enabled in BIOS
```
...
Extended feature bits (EAX=07H, ECX=0H)
eax: 0 ebx: 29c6fbf ecx: 0 edx: 0
sgx available: 1

CPUID Leaf 12H, Sub-Leaf 0 of Intel SGX Capabilities (EAX=12H,ECX=0)
eax: 1 ebx: 0 ecx: 0 edx: 241f
sgx 1 supported: 1
sgx 2 supported: 0
MaxEnclaveSize_Not64: 1f
MaxEnclaveSize_64: 24
...
```

That means that you are now able to call the special SGX calls of your CPU.  However you will always need the official Intel SGX Drivers including their Launch Enclave to initiate your own enclaves. To be able to execute SGX functions you need both `sgx available: 1` and `sgx [1|2] supported: 1`. Another hint that SGX functions are working is the output of an enclave size eg. `MaxEnclaveSize_64`. 

## Contribution

Add more hardware to this list via pull requests or simply via issues. 