# StargateTC: Redial

<img src="docs/images/flag-gb.svg" height="14" alt="EN"> **[English](#english)** &nbsp;·&nbsp; <img src="docs/images/flag-fr.svg" height="14" alt="FR"> **[Français](#francais)**

<img src="docs/images/flag-gb.svg" height="12" alt="EN"> StargateTC 2.0 running again on today's Steam Half-Life, with bots that play the objectives.

<img src="docs/images/flag-fr.svg" height="12" alt="FR"> StargateTC 2.0 qui retourne sur le Half-Life Steam d'aujourd'hui, avec des bots qui jouent les objectifs.

| | |
|---|---|
| ![A bot running into an open stargate](docs/images/gate-travel.jpg) | ![Apophis coming out of the temple gate](docs/images/temple-gate.jpg) |
| ![Bots at the DHD](docs/images/dhd.jpg) | ![Goa'uld fighting in the village](docs/images/village-fight.jpg) |

---

<a id="english"></a>
## <img src="docs/images/flag-gb.svg" height="20" alt=""> English

An unofficial patch for **StargateTC 2.0** (2007), the Stargate SG-1 mod for Half-Life:

- **It fixes the crashes** of the mod on the Half-Life 25th Anniversary update.
- **It adds bots** that dial the stargates, ride the rings, carry the key to the bomb and plant C4.

*Made with love by a former beta tester and amateur mapper from back in the glory days of StargateTC: [@CreepyInpu](https://x.com/CreepyInpu).*

### Quick start

1. Install **Half-Life** from Steam and start it once.
2. Download and install **StargateTC 1.0**: [direct link](https://www.moddb.com/downloads/start/7124). Install it into your `Half-Life` folder.
3. Download the **2.0 patch**: [direct link](https://www.moddb.com/downloads/start/8506). Extract it into `Half-Life\stargatetc`, overwriting the files.
4. Download **`stargatetc-redial-v1.0.1.zip`** from [Releases](../../releases), extract it anywhere, and double-click **`install.bat`**.
5. Restart Steam and launch **StargateTC 2.0** from your library. Then either:
   - **Find Servers > Create Server** (at least 10 players) to play with bots, or
   - join the 24/7 server: `connect 92.222.243.147:27030`.

Don't start the game between steps 3 and 4: StargateTC 2.0 crashes on today's Half-Life until Redial is installed.

### Installation details

- **The installer** finds Half-Life by itself, backs up every file it changes (`*.sgtcbots.bak`), and can be run again safely. Useful options:
  - `install.bat -GameDir "<path>\Half-Life\stargatetc"` if your game isn't found;
  - `install.bat -PackWeapons` for class weapon packs instead of point-buy.
- **To uninstall,** run **`uninstall.bat`**. To remove only the bots and keep the crash fixes, run `uninstall.bat -KeepFixes`.
- **Download checks** (MD5):
  - `SGTC_f1.0_full.exe`: `172c6ea866296a118be84b7cce697d82`;
  - `StargateTC_2.0_Eng_Patch.zip`: `b513e5f4f5408c67ce16fe7e9cfe191c`.
- **What the installer changes,** and how to do it by hand: [docs/DETAILS.md](docs/DETAILS.md).

### Play online

A server runs 24/7 with bots, so there's always a match: **`#eek server | StargateTC: Redial`**, `connect 92.222.243.147:27030`.

Bots leave as players join. When you join a server where only bots are playing, the round restarts so you're in right away.

### Controlling the bots

- **In game:** press `C`, then **Server Admin > Bots** on your own server, or **Rcon > Bots** on a dedicated one.
- **From the menu:** add a bot, set an automatic bot count, change the skill, or turn the chat on/off.
- **From the console:**

| Command | Effect |
|---|---|
| `jk_botti addbot` | add a bot |
| `jk_botti min_bots 4; jk_botti max_bots 12` | keep at least 4 bots and fill up to 12 players |
| `jk_botti botskill 1` … `5` | skill of new bots (1 = best) |
| `kick "<name>"` | remove a bot |

The bots added at each map start are listed in `stargatetc\addons\jk_botti\jk_botti.cfg`.

### If something goes wrong

- **The game closes at startup,** or shows "Assertion failed" when you join a team: run `install.bat` again.
- **There are no bots:** type `meta list` in the console. It should show `jk_botti` as `RUN`.
- **Bots stand still on a map:** that map has no waypoints. See the map list in [docs/DETAILS.md](docs/DETAILS.md#maps).

---

<a id="francais"></a>
## <img src="docs/images/flag-fr.svg" height="20" alt=""> Français

Un patch non officiel pour **StargateTC 2.0** (2007), le mod Stargate SG-1 de Half-Life :

- **il corrige les plantages** du mod sur la mise à jour 25e anniversaire de Half-Life ;
- **il ajoute des bots** qui composent les portes des étoiles, prennent les anneaux, portent la clé jusqu'à la bombe et posent du C4.

*Fait avec amour par un ancien bêta-testeur et mappeur amateur des grandes heures de StargateTC : [@CreepyInpu](https://x.com/CreepyInpu).*

### Démarrage rapide

1. Installez **Half-Life** depuis Steam et lancez-le une fois.
2. Téléchargez et installez **StargateTC 1.0** : [lien direct](https://www.moddb.com/downloads/start/7124). Installez-le dans votre dossier `Half-Life`.
3. Téléchargez le **patch 2.0** : [lien direct](https://www.moddb.com/downloads/start/8506). Extrayez-le dans `Half-Life\stargatetc` en écrasant les fichiers.
4. Téléchargez **`stargatetc-redial-v1.0.1.zip`** dans les [Releases](../../releases), extrayez-le n'importe où, puis double-cliquez sur **`install.bat`**.
5. Redémarrez Steam et lancez **StargateTC 2.0** depuis votre bibliothèque. Ensuite :
   - **Find Servers > Create Server** (au moins 10 joueurs) pour jouer avec des bots ;
   - ou rejoignez le serveur 24/7 : `connect 92.222.243.147:27030`.

Ne lancez pas le jeu entre les étapes 3 et 4 : StargateTC 2.0 plante sur le Half-Life actuel tant que Redial n'est pas installé.

### Détails d'installation

- **L'installeur** trouve Half-Life tout seul et sauvegarde chaque fichier qu'il modifie (`*.sgtcbots.bak`). On peut le relancer sans risque. Options utiles :
  - `install.bat -GameDir "<chemin>\Half-Life\stargatetc"` si le jeu n'est pas trouvé ;
  - `install.bat -PackWeapons` pour les packs d'armes par classe au lieu de l'achat à points.
- **Pour désinstaller,** lancez **`uninstall.bat`**. Pour retirer seulement les bots et garder les correctifs, lancez `uninstall.bat -KeepFixes`.
- **Vérification des téléchargements** (MD5) :
  - `SGTC_f1.0_full.exe` : `172c6ea866296a118be84b7cce697d82` ;
  - `StargateTC_2.0_Eng_Patch.zip` : `b513e5f4f5408c67ce16fe7e9cfe191c`.
- **Ce que l'installeur modifie,** et comment le faire à la main : [docs/DETAILS.md](docs/DETAILS.md) (en anglais).

### Jouer en ligne

Un serveur tourne 24h/24 avec des bots, donc il y a toujours une partie : **`#eek server | StargateTC: Redial`**, `connect 92.222.243.147:27030`.

Les bots cèdent leur place aux joueurs. Si vous rejoignez un serveur où seuls des bots jouent, la manche redémarre pour que vous entriez tout de suite.

### Contrôler les bots

- **En jeu :** appuyez sur `C`, puis **Server Admin > Bots** sur votre propre serveur, ou **Rcon > Bots** sur un serveur dédié.
- **Depuis le menu :** ajouter un bot, régler un nombre automatique de bots, changer leur niveau, activer ou couper leur chat.
- **Depuis la console :**

| Commande | Effet |
|---|---|
| `jk_botti addbot` | ajoute un bot |
| `jk_botti min_bots 4; jk_botti max_bots 12` | garde au moins 4 bots et complète jusqu'à 12 joueurs |
| `jk_botti botskill 1` … `5` | niveau des nouveaux bots (1 = le meilleur) |
| `kick "<nom>"` | retire un bot |

Les bots ajoutés à chaque début de carte sont listés dans `stargatetc\addons\jk_botti\jk_botti.cfg`.

### En cas de problème

- **Le jeu se ferme au démarrage,** ou affiche « Assertion failed » quand vous rejoignez une équipe : relancez `install.bat`.
- **Pas de bots :** tapez `meta list` dans la console. Il doit afficher `jk_botti` en `RUN`.
- **Les bots restent immobiles sur une carte :** elle n'a pas de waypoints. Voir la liste des cartes dans [docs/DETAILS.md](docs/DETAILS.md#maps).

---

## Credits

- **StargateTC: Redial:** [@CreepyInpu](https://x.com/CreepyInpu). Bug reports go to this repository's issues.
- **StargateTC:** the Stargate TC team; version 2.0 (2007) was released by Endolaylith.
- **jk_botti:** Jussi Kivilinna, maintained by [Bots-United](https://github.com/Bots-United/jk_botti). It is based on HPB bot by botman.
- **Metamod-P:** Will Day and Jussi Kivilinna ([metamod-p](https://github.com/Bots-United/metamod-p)).
- **Original waypoints and bot names:** Freeman551's 2012 ["Bots for StargateTC"](https://www.moddb.com/mods/stargatetc/downloads/bots-for-stargatetc) pack.
- **Half-Life:** Valve. **Stargate SG-1** is a trademark of MGM.

This is a fan project, not affiliated with the StargateTC team, Valve or MGM. You need your own copies of Half-Life and StargateTC; this repository does not redistribute the mod. License: see [LICENSE](LICENSE). Building from source: [docs/BUILDING.md](docs/BUILDING.md).
