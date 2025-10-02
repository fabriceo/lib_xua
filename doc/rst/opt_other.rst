|newpage|

Other options
=============

There are a few other, lesser used, options available. These are shown in :numref:`opt_other_defines`.

|beginfullwidth|

.. _opt_other_defines:

.. list-table:: Other defines
   :header-rows: 1
   :widths: 40 80 20

   * - Define
     - Description
     - Default
   * - ``XUA_USB_EN``
     - Allows the use of the audio subsystem without USB
     - ``1`` (enabled)
   * - ``INPUT_VOLUME_CONTROL``
     - Enables volume control on input channels, both descriptors and processing
     - ``1`` (enabled)
   * - ``OUTPUT_VOLUME_CONTROL``
     - Enables volume control on output channels, both descriptors and processing
     - ``1`` (enabled)
   * - ``XUA_CHAN_BUFF_CTRL``
     - Enables event based communication between XUA_Buffer_Ep() and XUA_Buffer_Decouple()
       which significantly reduces power consumption (approx 40 mW on xcore.ai) at the cost
       of consuming two extra channel-ends. Consequently this option may not be viable on
       some high end configurations which feature multiple digital interfaces such as SPDIF
       or ADAT.
     - ``0`` (disabled)
   * - ``XUA_USER_FUNCTION_CALL_PRE_BUFFER``
     - Can be used to insert code to be run before buffer is started. This may be useful
       for cases where global variables need to be initialised (eg. global channel or interface).
     -  Undefined
   * - ``XUA_USER_IN_ENDPOINTS``
     - Allows additional input USB endpoints to be declared. Endpoints must be initialised using code
       from the ``XUA_USER_EP_INIT`` define.
     - Undefined
   * - ``XUA_USER_OUT_ENDPOINTS``
     - Allows additional output USB endpoints to be declared. Endpoints must be initialised using code
       from the ``XUA_USER_EP_INIT`` define.
     - Undefined
   * - ``XUA_USER_EP_INIT``
     - Used for inserting code to initialise additional user endpoints. Eg.
       ``#define XUA_USER_EP_INIT        epTypeTableOut[ENDPOINT_NUMBER_OUT_BULK_CDC] = XUD_EPTYPE_INT;``
     - Undefined


  
|endfullwidth|
