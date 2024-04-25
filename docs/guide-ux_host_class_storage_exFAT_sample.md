# Purpose
* Build USBX Host Storage Example from existing GitHub Example, which
  * exFAT support

# Steps
## Get MIMXRT1060 (IAR) Examples
* Download: https://github.com/azure-rtos/samples/releases/download/v6.1_rel/Azure_RTOS_6.1_MIMXRT1060_IAR_Samples_2021_11_03.zip
* Extract
* Confirm project `sample_usbx_host_mass_storage` works

## Add exFAT support
* Add `FX_ENABLE_EXFAT` definition in these lib projects/app project
  * usbx
  * filex
  * sample_usbx_host_mass_storage
* Re-compile projects
* Run `sample_usbx_host_mass_storage`

## Improve disconnect detect:
```c
    status =  ux_host_stack_class_get(_ux_system_host_class_storage_name, &class);
    if (status != UX_SUCCESS)
        return(status);
```
```c
    status =  ux_host_stack_class_get(_ux_system_host_class_storage_name, &class);
    if (status != UX_SUCCESS)
    {
        storage = UX_NULL;
        media = UX_NULL;
        return(status);
    }
```

## Improve debug
Add to `Live Watch` to see value changes:
```
storage
media
file_name
local_buffer
```