# FERSAT PDH
## 12. commit
Napisao samo prekidnu podrutinu za I2C po uzoru na prethodnika i iskomentirao dio koda kojega smatram konačnim.
## 11. commit
Napokon sam shvatil kak I2C funkcionira ovdje, uspio sam pročitati chip ID sa kamere.

Postoji samo jedna nejasnoća:
Kada sam provjeraval TXE bit u registru ISR, `while(READ_BIT(I2Cx->ISR, I2C_ISR_TXE) != 1)` je funkcioniralo sasvim dobro,
no kada sam na taj isti način išao provjeravati RXNE bit u registru ISR, `while(READ_BIT(I2Cx->ISR, I2C_ISR_RXNE) != 1)`, program je
zapeo u while petlji, a RXNE je je bio u jedinici. Kod provjere RXNE-a sam koristil način koji sam naučio na URS-u,
`while(!((I2Cx->ISR) & I2C_ISR_RXNE))`, i onda je sve bilo dobro. There are fell voices on the air.
## 10. commit
SVE KAJ NE RAZMEM I KAJ ME ZBUNJUJE SAM POBRISAL.
## 9. commit
Strane sile su još jednom podbacile u svojem pokušaju da potkopaju Hrvatski Svemirski Program. Uspio sam ostvariti SPI komunikaciju sa kamerom.
I2C još nisam uspio, ali kad je SPI radio onda bude valja i I2C.
## 8. commit
Sad kad sam se prestal praviti pametan i nisam prčkal po izgeneriranom kodu uspio sam uspješno pročitati device ID od flash memorije.
12. 04. 2022. bude zapamćen kao povjesni datum.
## 7. commit
Ponovno sam izgeneriral kod jer sam se zbunil. Bilješka za mene: Ispada da nije baš pametno u tijeku testiranja prčkati po izgeneriranom kodu, jer budeš DEFINITIVNO još trebal code generator.
## 6. commit
Dodatno uređen kod. Izbrisan nepotreban generiran kod, dodana inicijalizacijska funkcija GPIO pinova za debug.
## 5. commit
Napravljene posebne funkcije za inicijalizaciju GPIO pinova. Izbrisan dio izgeneriranog koda.
## 4. commit
Dodana delay funkcionalnost
## 3. commit
Modificiran .gitignore
## 2. commit
Izbrisan \Debug.
## 1. commit
Inicijaliziran projekt. Pocetak rada na flash memoriji.