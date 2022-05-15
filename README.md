# FERSAT PDH
## 17. commit
Prepisao i modificirao taskove za FreeRTOS.
## 16. commit
Prepisao i modificirao kod za DMA prijenos. Kod za kameru je sada do kraja prepisan. Krećem na prepisivanje taskova za FreeRTOS.
## 15. commit
Prepisao sam kod za flash memoriju, inicijalizirao sam FreeRTOS i DMA, trenutačno prepisujem kod za kameru. Nisam ništa pokušao testirati do sada.
Za sada ću prepisivati kod i nadati se najboljem i kada prepišem sve što mi treba sređivat ću kod da radi na ovom mikrokontroleru.
## 14. commit
Kratki update.

Shvatil sam zakaj kod I2C komunikacije kada sam provjeraval RXNE zastavicu nije funkcioniralo
`while(READ_BIT(I2Cx->ISR, I2C_ISR_RXNE) != 1)`. Dakle, funkcija `READ_BIT` radi tak da napravi logički I
ISR registra i pozicije zastavice koja nas zanima, dakle `((REG) & (BIT))`, i onda zapravo dobijemo 32-bitni broj koji
ima jedinicu ili nulu na mjestu zastavice koja nas zanima i ima 0 na ostalim mjestima, a ja genijalac sam mislil da jednostavno
dobijem samo nulu ili jedinicu kao povratnu informaciju. Zastavica TXE je na najnižem mjestu u ISR registru i zato, kada je ta zastavica
bila podignuta, mogel sam registar uspoređivati sa jedinicom, jer sam u tom slučaju kao povratnu informaciju dobil upravo jedinicu.
Da bi na isti način mogel ispitivati i RXNE zastavicu, moral sam pročitanu vrijednost uspoređivati sa brojem 4, jer se ta zastavica nalazi
na drugom bitu u ISR registru.
## 13. commit
Uf, broj 13. Sva sreća pa nisam praznovjerna osoba *Kaže to dok kuca u stol i pokušava ne ubiti pauka koji mu daje noćne more*.

Uglavnom, mislim da je I2C komunikacija za kameru napravljena. Moguće je da nije, s obzirom na to da se bude potpuni test napravil tek
kada napravim sve ostalo, ali ne vidim razloga za probleme. Sad krećem s radom na flash memoriji, pa nakon toga budem radil na kameri i tek
nakon toga budem implementiral RTOS. Kad sve to obavim, organiziral budem kod da to bude kak Bog zapovijeda.
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