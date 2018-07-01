rm *.bmp

convert -type truecolor -crop 800x128+0+19 1.png BMP3:header.bmp
convert -rotate 270 header.bmp BMP3:header_rotated.bmp

convert -type truecolor -crop 192x64+0+294 1.png BMP3:icon_wifi.bmp
convert -rotate 270 icon_wifi.bmp BMP3:icon_wifi_rotated.bmp

convert -type truecolor -crop 192x64+0+294 2.png BMP3:icon_nowifi.bmp
convert -rotate 270 icon_nowifi.bmp BMP3:icon_nowifi_rotated.bmp

convert -type truecolor -crop 416x64+192+294 1.png BMP3:wifi.bmp
convert -rotate 270 wifi.bmp BMP3:wifi_rotated.bmp

convert -type truecolor -crop 192x64+0+358 1.png BMP3:icon_bt.bmp
convert -rotate 270 icon_bt.bmp BMP3:icon_bt_rotated.bmp

convert -type truecolor -crop 192x64+0+358 2.png BMP3:icon_nobt.bmp
convert -rotate 270 icon_nobt.bmp BMP3:icon_nobt_rotated.bmp

convert -type truecolor -crop 416x64+192+358 1.png BMP3:bt.bmp
convert -rotate 270 bt.bmp BMP3:bt_rotated.bmp

convert -type truecolor -crop 192x64+0+422 1.png BMP3:icon_cellular.bmp
convert -rotate 270 icon_cellular.bmp BMP3:icon_cellular_rotated.bmp

convert -type truecolor -crop 192x64+0+422 2.png BMP3:icon_nocellular.bmp
convert -rotate 270 icon_nocellular.bmp BMP3:icon_nocellular_rotated.bmp

convert -type truecolor -crop 416x64+192+422 1.png BMP3:cellular.bmp
convert -rotate 270 cellular.bmp BMP3:cellular_rotated.bmp

convert -type truecolor -crop 192x64+608+294 1.png BMP3:switch_en_on.bmp
convert -rotate 270 switch_en_on.bmp BMP3:switch_en_on_rotated.bmp

convert -type truecolor -crop 192x64+608+358 1.png BMP3:switch_en_on_w.bmp
convert -rotate 270 switch_en_on_w.bmp BMP3:switch_en_on_w_rotated.bmp

convert -type truecolor -crop 192x64+608+294 2.png BMP3:switch_dis_off.bmp
convert -rotate 270 switch_dis_off.bmp BMP3:switch_dis_off_rotated.bmp

convert -type truecolor -crop 192x64+608+358 2.png BMP3:switch_dis_off_w.bmp
convert -rotate 270 switch_dis_off_w.bmp BMP3:switch_dis_off_w_rotated.bmp

convert -type truecolor -crop 192x64+608+294 3.png BMP3:switch_dis_on.bmp
convert -rotate 270 switch_dis_on.bmp BMP3:switch_dis_on_rotated.bmp

convert -type truecolor -crop 192x64+608+358 3.png BMP3:switch_dis_on_w.bmp
convert -rotate 270 switch_dis_on_w.bmp BMP3:switch_dis_on_w_rotated.bmp

convert -type truecolor -crop 192x64+608+294 4.png BMP3:switch_en_off.bmp
convert -rotate 270 switch_en_off.bmp BMP3:switch_en_off_rotated.bmp

convert -type truecolor -crop 192x64+608+358 4.png BMP3:switch_en_off_w.bmp
convert -rotate 270 switch_en_off_w.bmp BMP3:switch_en_off_w_rotated.bmp

convert -type truecolor -crop 192x64+0+566 1.png BMP3:icon_camera.bmp
convert -rotate 270 icon_camera.bmp BMP3:icon_camera_rotated.bmp

convert -type truecolor -crop 192x64+0+566 2.png BMP3:icon_nocamera.bmp
convert -rotate 270 icon_nocamera.bmp BMP3:icon_nocamera_rotated.bmp

convert -type truecolor -crop 416x64+192+566 1.png BMP3:camera.bmp
convert -rotate 270 camera.bmp BMP3:camera_rotated.bmp

convert -type truecolor -crop 192x64+0+630 1.png BMP3:icon_speaker.bmp
convert -rotate 270 icon_speaker.bmp BMP3:icon_speaker_rotated.bmp

convert -type truecolor -crop 192x64+0+630 2.png BMP3:icon_nospeaker.bmp
convert -rotate 270 icon_nospeaker.bmp BMP3:icon_nospeaker_rotated.bmp

convert -type truecolor -crop 416x64+192+630 1.png BMP3:speaker.bmp
convert -rotate 270 speaker.bmp BMP3:speaker_rotated.bmp

convert -type truecolor -crop 192x64+0+694 1.png BMP3:icon_mic.bmp
convert -rotate 270 icon_mic.bmp BMP3:icon_mic_rotated.bmp

convert -type truecolor -crop 192x64+0+694 2.png BMP3:icon_nomic.bmp
convert -rotate 270 icon_nomic.bmp BMP3:icon_nomic_rotated.bmp

convert -type truecolor -crop 416x64+192+694 1.png BMP3:mic.bmp
convert -rotate 270 mic.bmp BMP3:mic_rotated.bmp

convert -type truecolor -crop 192x64+0+840 1.png BMP3:icon_gps.bmp
convert -rotate 270 icon_gps.bmp BMP3:icon_gps_rotated.bmp

convert -type truecolor -crop 192x64+0+840 2.png BMP3:icon_nogps.bmp
convert -rotate 270 icon_nogps.bmp BMP3:icon_nogps_rotated.bmp

convert -type truecolor -crop 416x64+192+840 1.png BMP3:gps.bmp
convert -rotate 270 gps.bmp BMP3:gps_rotated.bmp

convert -type truecolor -crop 192x64+0+904 1.png BMP3:icon_sensor.bmp
convert -rotate 270 icon_sensor.bmp BMP3:icon_sensor_rotated.bmp

convert -type truecolor -crop 192x64+0+904 2.png BMP3:icon_nosensor.bmp
convert -rotate 270 icon_nosensor.bmp BMP3:icon_nosensor_rotated.bmp

convert -type truecolor -crop 416x64+192+904 1.png BMP3:sensor.bmp
convert -rotate 270 sensor.bmp BMP3:sensor_rotated.bmp

convert -type truecolor -crop 96x64+111+148 1.png BMP3:mode_none.bmp
convert -rotate 270 mode_none.bmp BMP3:mode_none_rotated.bmp

convert -type truecolor -crop 128x64+270+148 1.png BMP3:mode_airplane.bmp
convert -rotate 270 mode_airplane.bmp BMP3:mode_airplane_rotated.bmp

convert -type truecolor -crop 96x64+432+148 1.png BMP3:mode_movie.bmp
convert -rotate 270 mode_movie.bmp BMP3:mode_movie_rotated.bmp

convert -type truecolor -crop 96x64+591+148 1.png BMP3:mode_stealth.bmp
convert -rotate 270 mode_stealth.bmp BMP3:mode_stealth_rotated.bmp

convert -type truecolor -crop 800x64+0+967 1.png BMP3:bluerect.bmp
convert -rotate 270 bluerect.bmp BMP3:bluerect_rotated.bmp

convert -type truecolor -crop 192x64+0+230 1.png BMP3:group_networking.bmp
convert -rotate 270 group_networking.bmp BMP3:group_networking_rotated.bmp

convert -type truecolor -crop 192x64+0+502 1.png BMP3:group_multimedia.bmp
convert -rotate 270 group_multimedia.bmp BMP3:group_multimedia_rotated.bmp

convert -type truecolor -crop 192x64+0+776 2.png BMP3:group_sensor.bmp
convert -rotate 270 group_sensor.bmp BMP3:group_sensor_rotated.bmp

convert -type truecolor -crop 64x64+229+230 1.png BMP3:group_en_on.bmp
convert -rotate 270 group_en_on.bmp BMP3:group_en_on_rotated.bmp

convert -type truecolor -crop 64x64+388+230 1.png BMP3:group_en_off.bmp
convert -rotate 270 group_en_off.bmp BMP3:group_en_off_rotated.bmp

convert -type truecolor -crop 128x64+549+230 1.png BMP3:group_en_custom.bmp
convert -rotate 270 group_en_custom.bmp BMP3:group_en_custom_rotated.bmp

convert -type truecolor -crop 64x64+229+230 5.png BMP3:group_dis_on.bmp
convert -rotate 270 group_dis_on.bmp BMP3:group_dis_on_rotated.bmp

convert -type truecolor -crop 64x64+388+230 5.png BMP3:group_dis_off.bmp
convert -rotate 270 group_dis_off.bmp BMP3:group_dis_off_rotated.bmp

convert -type truecolor -crop 128x64+549+230 5.png BMP3:group_dis_custom.bmp
convert -rotate 270 group_dis_custom.bmp BMP3:group_dis_custom_rotated.bmp

convert -type truecolor -crop 32x64+80+148 1.png BMP3:mode_select.bmp
convert -rotate 270 mode_select.bmp BMP3:mode_select_rotated.bmp

convert -type truecolor -crop 32x64+235+148 1.png BMP3:mode_deselect.bmp
convert -rotate 270 mode_deselect.bmp BMP3:mode_deselect_rotated.bmp

convert -type truecolor -crop 32x64+198+230 1.png BMP3:group_en_deselect.bmp
convert -rotate 270 group_en_deselect.bmp BMP3:group_en_deselect_rotated.bmp

convert -type truecolor -crop 32x64+523+230 1.png BMP3:group_en_select.bmp
convert -rotate 270 group_en_select.bmp BMP3:group_en_select_rotated.bmp

convert -type truecolor -crop 32x64+359+230 5.png BMP3:group_dis_select.bmp
convert -rotate 270 group_en_select.bmp BMP3:group_dis_select_rotated.bmp

convert -type truecolor -crop 32x64+523+230 5.png BMP3:group_dis_deselect.bmp
convert -rotate 270 group_dis_deselect.bmp BMP3:group_dis_deselect_rotated.bmp

convert -type truecolor -crop 640x128+99+1051 2.png BMP3:footer.bmp
convert -rotate 270 footer.bmp BMP3:footer_rotated.bmp
