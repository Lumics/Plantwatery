# Plantwatery

A project of Anna Dai and Maurin Widmer.

<img src="images/plantwatery-5.jpg" width="800">

## Summary

Plantwatery is an automated watering system for your garden plants. It is based on an ESP32, a capacitive soil moisture sensor, solar cell & battery and a water pump. The system is autonomous and measures twice per day the soil moisture and operates the pump if needed. It's collected data and status is send via Wifi (MQTT protocol) to the cloud. All parts can easily be bought and the case is 3D printied (drawings are provided). The plastic housing is constructed to protect it's inner electronic parts from water and makes it easy to attach the solar panel, water pump and soil sensor.

The code is written with the Arduino framework and can easily be attapted to other platforms. 
It allows to change watering time & hours of the day, soil sensor offset and MQTT topics. Furthermore, the code automatically checks if a new version is online and updates itself if there is (for example because you want to change some parameters like soil sensor offset or watering time). 

## Hardware

<img src="images/plantwatery-6.jpg" width="800">

In the following you have a list of suggestions of parts you need to build this project. 

### Control unit:
* Microcontroller (I know that there are better ESP32 for low power applications than the TTGO T-Display but I really like the board and had just this one available with a battery connector): [ESP32 with battery connector](https://www.aliexpress.com/item/33048962331.html?spm=a2g0o.productlist.0.0.b58c2c098F7mfN&algo_pvid=9a81adc2-ce0c-47bb-a4dd-71a462476a64&algo_expid=9a81adc2-ce0c-47bb-a4dd-71a462476a64-10&btsid=0bb0623f16201391171217936ea511&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* Relay or Mosfet to control pump: [IRF520N Mosfet board](https://www.aliexpress.com/item/4000522397541.html?spm=a2g0o.productlist.0.0.65f26e63slLK2D&algo_pvid=0c848908-41f4-49be-a38d-7ed1e34a9059&algo_expid=0c848908-41f4-49be-a38d-7ed1e34a9059-0&btsid=0b0a556816201292746752554e52cc&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* Battery Management Board: [TP4056](https://www.aliexpress.com/item/4000522397541.html?spm=a2g0o.productlist.0.0.de4b58c9qydgt6&algo_pvid=69905307-5800-4be5-9b2b-80f6b985bcc4&algo_expid=69905307-5800-4be5-9b2b-80f6b985bcc4-0&btsid=0bb0623a16201391745902844e4ca4&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* Battery holder: [18650 Case](https://www.aliexpress.com/item/1005001707889794.html?spm=a2g0o.productlist.0.0.1e6d1e3dfwBJik&algo_pvid=513f1515-1784-40b9-8499-b16564c27130&algo_expid=513f1515-1784-40b9-8499-b16564c27130-0&btsid=0b0a557016201391231408758e1076&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* Battery: [18650 Lithium Rechargeable Battery 3.7V](https://www.aliexpress.com/item/32324914059.html?spm=a2g0o.productlist.0.0.27c22142u1oYY9&algo_pvid=61708288-d49a-48cc-b7a1-7b9bbae778fb&algo_expid=61708288-d49a-48cc-b7a1-7b9bbae778fb-1&btsid=0b0a555316201390081524110edf99&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* PCB Prototype board to solder the wires together and add the connectors: [PCB Prototype Board](https://www.aliexpress.com/item/32588853051.html?spm=a2g0o.productlist.0.0.690f190cdSvO5x&algo_pvid=8745a321-eece-4b08-916e-bb816e874ff8&algo_expid=8745a321-eece-4b08-916e-bb816e874ff8-0&btsid=0b0a555916201387423844532e53c2&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* Cable connector: [PCB Screw Terminal](https://www.aliexpress.com/item/1000006518504.html?spm=a2g0o.productlist.0.0.753553808svNSN&algo_pvid=null&algo_expid=null&btsid=0b0a556216201400093754778e0179&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* Jumper cables: [Dupont Jumper Wire](https://www.aliexpress.com/item/32911287776.html?spm=a2g0o.detail.0.0.7f875552q3YbrB&gps-id=pcDetailBottomMoreThisSeller&scm=1007.13339.169870.0&scm_id=1007.13339.169870.0&scm-url=1007.13339.169870.0&pvid=9e77f922-b1ed-43af-a189-ed38dffac609&_t=gps-id:pcDetailBottomMoreThisSeller,scm-url:1007.13339.169870.0,pvid:9e77f922-b1ed-43af-a189-ed38dffac609,tpp_buckets:668%230%23131923%2362_668%230%23131923%2362_668%23888%233325%235_668%23888%233325%235_668%232846%238110%231995_668%235811%2327182%2353_668%232717%237560%23222_668%231000022185%231000066059%230_668%233468%2315608%23187_668%232846%238110%231995_668%235811%2327182%2353_668%232717%237560%23222_668%233164%239976%23952_668%233468%2315608%23187)
and some [longer cables](https://www.aliexpress.com/item/4000009001537.html?spm=a2g0o.productlist.0.0.5fd4257fB5iarJ&algo_pvid=1c5f1618-e462-4103-8249-88205578790c&algo_expid=1c5f1618-e462-4103-8249-88205578790c-0&btsid=0b0a555516201388513911642e8ba7&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* 3D printed housing

### Solar part:
* [Solar cell 5V, 200mA](https://www.aliexpress.com/item/32906698984.html?spm=a2g0o.productlist.0.0.7e174646s3atFa&algo_pvid=ffe7d644-ad29-4754-aad0-a4e805e1195d&algo_expid=ffe7d644-ad29-4754-aad0-a4e805e1195d-0&btsid=0b0a557216201399178301207e1874&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* 3D printed housing
* Some hot glue to make it waterproof
* Wooden stick to mount the housing

### Soil measuring part:

* Soil sensor: [Capacitive Soil Sensor](https://www.aliexpress.com/item/32832538686.html?spm=a2g0o.productlist.0.0.70be1298CuZCbd&algo_pvid=03a71152-fe64-495e-baa5-ce52589335b6&algo_expid=03a71152-fe64-495e-baa5-ce52589335b6-0&btsid=0b0a556c16201375263552257ec580&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* Some hot glue to make the electronics part waterproof


### Water part:

* Water pump, around 5V but they also work with 3.7V: [Mini Brushless Water Pump 5V, 2.4W](https://www.aliexpress.com/item/32995965702.html?spm=a2g0o.productlist.0.0.148213d7WiiVVc&algo_pvid=42949def-7ded-44ff-baf7-55eefb6475f7&algo_expid=42949def-7ded-44ff-baf7-55eefb6475f7-5&btsid=0b0a556a16201394058158545e4a43&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* Tubes, I got the 6mm (outside) as they fit perfectly inside of the pump fixture: [Tubes](https://www.aliexpress.com/item/4000859747207.html?spm=a2g0s.9042311.0.0.7d434c4d9Ae92m) 
* Connectors for the tubes (6mm): [Pipe connector](https://www.aliexpress.com/item/4001338085412.html?spm=a2g0o.detail.1000060.1.d8916b4bOSQm3U&gps-id=pcDetailBottomMoreThisSeller&scm=1007.13339.169870.0&scm_id=1007.13339.169870.0&scm-url=1007.13339.169870.0&pvid=5e90a557-ecfb-47ad-835c-de327359bc74&_t=gps-id:pcDetailBottomMoreThisSeller,scm-url:1007.13339.169870.0,pvid:5e90a557-ecfb-47ad-835c-de327359bc74,tpp_buckets:668%230%23131923%2357_668%230%23131923%2357_668%23888%233325%235_668%23888%233325%235_668%232846%238110%231995_668%235811%2327180%2341_668%232717%237562%23492_668%231000022185%231000066059%230_668%233468%2315607%2396_668%232846%238110%231995_668%235811%2327180%2341_668%232717%237562%23492_668%233164%239976%23386_668%233468%2315607%2396)
* Stopper for the end of the tubes: [Stopper connector](https://www.aliexpress.com/item/4000027416210.html?spm=a2g0o.detail.0.0.4a0d454eggQtPm&gps-id=pcDetailBottomMoreThisSeller&scm=1007.13339.169870.0&scm_id=1007.13339.169870.0&scm-url=1007.13339.169870.0&pvid=22fbe9bb-979b-482c-955a-72fbf8ae5a17&_t=gps-id:pcDetailBottomMoreThisSeller,scm-url:1007.13339.169870.0,pvid:22fbe9bb-979b-482c-955a-72fbf8ae5a17,tpp_buckets:668%230%23131923%2357_668%230%23131923%2357_668%23888%233325%235_668%23888%233325%235_668%232846%238110%231995_668%235811%2327180%2341_668%232717%237562%23492_668%231000022185%231000066059%230_668%233468%2315607%2396_668%232846%238110%231995_668%235811%2327180%2341_668%232717%237562%23492_668%233164%239976%23386_668%233468%2315607%2396)
* Large bucket
* Maybe some wooden sticks and some wire to hold the tubes in place


<img src="images/plantwatery-2.jpg" width="800">

## Electronics

Here is a quick sketch of the electronics components. It should be straight forward. Be ware that we combined common signals on the PCB and made it easy to attach the peripherals (not shown in sketch).

<img src="images/sketch.jpg" width="800">

## Watering system

The tubes were connected to the pump and then placed between the plants. We used a Y connector to go to all plants and always have a small tube which has some self made small holes at the spots where it should water the plants. Therefore, we only have to change the tubes with the holes, if we rearrange the plants or want suddendly smaller holes.


## Code
<img src="images/code_parameters.png" width="800">

To code is written for the Arduino framework with the PLatformIo IDE. PLease make sure to add a file called: "credentials.h" to the src folder (copy and rename the credentials_example.h and fill in the details). If you do not have a OTA server you can just leave the URL like it is or remove everything with OTA from the code. 

Make sure to change the hum_threshold, hum_offset and the watering times according to your needs.

<img src="images/plantwatery-3.jpg" width="800">

Have fun with the project and let us know if you have a question or if you build this. 