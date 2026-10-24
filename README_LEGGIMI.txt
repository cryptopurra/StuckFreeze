STUCKFREEZE - Stutter + Spectral Freeze (VST3)
==============================================

COME OTTENERE IL PLUGIN (senza installare nulla)
------------------------------------------------
1. Crea un account gratuito su https://github.com
2. Clicca "+" in alto a destra -> "New repository". Nome: StuckFreeze. Lascialo Public. "Create repository".
3. Nella pagina del repository clicca "uploading an existing file".
4. Decomprimi lo zip e TRASCINA DENTRO IL CONTENUTO della cartella StuckFreeze
   (CMakeLists.txt, cartella Source, cartella .github, ecc.).
   - Su Mac la cartella ".github" e' nascosta: premi Cmd+Shift+. nel Finder per vederla.
   - Se la cartella .github non si carica, in GitHub fai "Add file -> Create new file",
     scrivi come nome  .github/workflows/build.yml  e incolla il contenuto del file build.yml.
5. Clicca "Commit changes".
6. Vai nella scheda "Actions": parte da solo "Build StuckFreeze" (10-15 minuti).
7. Quando e' verde, aprilo e in fondo, sotto "Artifacts", scarica
   StuckFreeze-windows-latest  oppure  StuckFreeze-macos-latest.
8. Copia la cartella StuckFreeze.vst3 in:
   Windows:  C:\Program Files\Common Files\VST3
   Mac:      ~/Library/Audio/Plug-Ins/VST3   (e se il Mac lo blocca: tasto destro > Apri,
             oppure da Terminale:  xattr -cr StuckFreeze.vst3)
9. Riscansiona i plugin nel tuo DAW.
   Nell'artifact c'e' anche una versione Standalone per provarlo senza DAW.

SE LA COMPILAZIONE DA' ERRORE (e' un codice nuovo, mai compilato da me):
apri il passaggio rosso in Actions, copia il testo dell'errore e incollalo a Claude.

COMANDI
-------
STUTTER: On, Sync (a tempo del DAW) / Free (ms), Length 1/1..1/16, Smooth, Pitch,
         Reverse, Low cut.
         KEY: scegli una nota (C..B) e KEY OCT (ottava): il loop prende la lunghezza
         di un periodo di quella nota e diventa un suono intonato. OFF = disattivato.
         Attivalo al momento giusto: ripete l'ultimo pezzo di audio appena passato.
FREEZE:  On (congela lo spettro), Smear, Pitch, Reducer (bit), Freeze mix.
MASTER:  Routing (Stutter>Freeze, Freeze>Stutter, Parallelo), Mix, Output.
Automatizza i pulsanti On dal DAW per fare drop ed effetti ritmici.

INTERFACCIA: ridimensionabile trascinando l'angolo in basso a destra della finestra.
