# BiKayaOS

## Regole per Claude: PR, release e branch

Valgono per tutti i repo di `davidetinti` e `davidetinti-apps`.

- Claude può aprire le PR, unirle quando i check sono verdi e unire anche la PR di release (release-please) senza chiedere conferma.
- Dopo il merge di una PR, i branch confluiti vanno eliminati, in locale e sul remoto (`git branch -d`, `git push origin --delete`). Il repo ha attiva anche l'eliminazione automatica dei branch dopo il merge.
- Nei comandi `gh` metti `--repo owner/repo` subito dopo il sottocomando (es. `gh pr merge --repo owner/repo 12 --merge`): solo così combaciano con i permessi di `.claude/settings.json`.
