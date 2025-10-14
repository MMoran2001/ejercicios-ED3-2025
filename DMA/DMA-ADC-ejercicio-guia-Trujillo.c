/*
Adquisición Cíclica de Alta Frecuencia con ADC (Periférico → Memoria)
Consigna: Desarrolle un sistema de adquisición de datos para una señal de alta frecuencia que garantice
un muestreo continuo y cíclico.
Lógica:
• Configure el ADC para muestrear a una alta frecuencia, por ejemplo, 60 kHz, en modo burst o activado
por un Timer.
• Configure un canal de DMA para transferir las muestras del ADC al SRAM Bank 1.
• El buffer de destino debe ser cíclico (circular).
*/

// HACER