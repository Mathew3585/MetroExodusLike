# MetroExodusLike — Roadmap Vertical Slice

**Objectif :** une vertical slice solo, jouable du début à la fin, d'environ 15-20 min, qui donne « l'effet Metro ».
Un tunnel + une station, des humains et quelques mutants, la survie avec masque à gaz, l'infiltration à la lumière, le crafting de base.

**Règles du projet**
- Toute la logique en C++. Les Blueprints servent **uniquement** de fichiers de données (mesh, sons, valeurs).
- Une étape = on code → on compile → **on teste en jeu** → on valide le critère « Fini quand » → étape suivante.
- On ne commence jamais une étape tant que la précédente n'est pas validée.
- Greybox (cubes) d'abord, assets achetés seulement quand une mécanique fonctionne.
- Solo uniquement, pas de réseau.

---

## Phase 0 — Fondations
- [x] 0.1 Git + `.gitignore` Unreal (+ LFS pour `Content/`)
- [x] 0.2 Arborescence `Source/` par domaine (Core, Player créés ; Weapons, AI, Survival, Interaction, Inventory créés quand on en a besoin)
- [x] 0.3 Build.cs : modules nécessaires (EnhancedInput, AIModule, GameplayTasks, NavigationSystem, UMG, PhysicsCore) + log `LogME`
- [x] 0.4a `AMEGameMode`, `AMEPlayerController`, `AMECharacter` (caméra FP) + GameMode par défaut dans la config
- [x] 0.4b Map de test greybox `Content/Maps/L_Test` (aujourd'hui `MetroExodusLike/Maps/L_Test`) (tunnel, escalier, rampe, caisses de saut, passage bas)

**Fini quand :** le projet compile, PIE démarre sur la map de test avec notre GameMode.

## Phase 1 — Le corps du joueur (true FPS)
Choix : **true FPS** — un seul corps complet (`SKM_QuantumCharacter_NoHead`), caméra dans la tête, on voit ses jambes.
Phase 3 : passage en **hybride** — bras FPS `SKM_Arms` sur la caméra pour l'arme, le corps garde jambes/torse (bras cachés), l'ombre reste complète.

- [x] 1.1 Enhanced Input : Input Actions + Mapping Context (assets) référencés par un DataAsset, bindés en C++
- [x] 1.2 Marcher / courir / sauter / regarder (testable avec la capsule seule)
- [x] 1.3 Corps true FPS : mesh NoHead, caméra sur le bone de la tête, `UMEAnimInstance` en C++ + AnimBP (graphe uniquement)
- [x] 1.4 Stabilisation de la caméra (filtrer le mouvement de tête de l'anim)
- [x] 1.5 Locomotion complète via Game Animation Sample (gratuit, retargeté) : marche arrière, strafe, rotation sur place
- [x] 1.6 S'accroupir (transition de caméra fluide)
- [x] 1.7 Poids Metro : rotation sur place, atterrissages léger/lourd (arrêt net + regard au sol), inclinaison en strafe
- [x] 1.8 Bruits de pas (`UMEFootstepComponent` + `DA_Footsteps`) : pas détectés sur les pieds du corps (pas de notifies), son par Physical Material (FootstepsMiniPack), volume marche / sprint / accroupi, son de réception des sauts et chutes ; le bruit pour l'IA viendra en 5.2

**Fini quand :** se déplacer est agréable et « lourd », et on voit son corps de façon crédible en regardant en bas.

## Phase 2 — Interaction
Règle Metro : **toujours armé** (pas de touche pour ranger l'arme, sauf zones sûres scriptées).
Toutes les interactions se font **main gauche**, l'arme reste en main droite. Chaque interactable déclare son
geste (ramasser, porte, interrupteur…) et sa durée : l'animation main gauche s'y branchera en Phase 3.

- [x] 2.1 Interface `IMEInteractable` (geste + durée de maintien) + `UMEInteractionComponent` (trace depuis la caméra, focus, maintien)
- [x] 2.2 Prompt minimal à l'écran (« [E] Ouvrir » + barre de maintien), widget construit en C++
- [x] 2.3 Premiers interactables : porte (s'ouvre à l'opposé du joueur), objet ramassable, interrupteur / disjoncteur (maintien)

**Fini quand :** on peut ouvrir une porte et ramasser un objet.

## Phase 3 — Première arme
- [x] 3.0 Retarget des animations du pack AR-15 (Manny → squelette Quantum, `ik_hand_gun` conservé)
- [x] 3.1 `UMEWeaponData` (DataAsset) + `AMEWeapon` + `UMEWeaponComponent`
- [x] 3.2 Bras FPS + AR-15 en main, montage de la démo du pack : pivot tête (suit la vue) → bras → caméra sur l'os `head` des bras ; animations du pack jouées telles quelles (pas de retarget), First Person Rendering, buste + jambes visibles, regard bas limité
- [x] 3.2w Arme près d'un mur : sphère depuis la caméra, l'arme passe par les poses « Close To Wall » du pack au lieu de traverser
- [x] 3.2a Bras de notre perso : bras MetaHuman (`SKM_Player_FPArms_Body/Outfit`) importés sur le squelette du pack (`SK_Mannequin`, os MH fusionnés), translation retargeting en Animation (proportions du pack), branchés dans `FirstPersonArms`
- [x] 3.2c Zones sûres (`AMESafeZone` / `BP_SafeZone`) : l'arme se range et reste rangée (pas de tir, visée, rechargement, réticule), ressort à la sortie ; medkit immédiat dedans
  - `L_DevMap` = terrain de test greybox (`Dev/…` dans l'Outliner) : stand de tir 10/20/30 m, murs béton / métal / bois, zone sûre avec porte + interrupteur + disjoncteur, ramassage, tours de chute 3 / 6 / 12 m
- [ ] 3.2b Gestes main gauche branchés sur les interactions (ramasser, porte, interrupteur)
- [x] 3.3 Tir hitscan depuis la vue (auto / semi, cadence exacte), munitions (chargeur + réserve), tir à vide, rechargement tactique / à vide, sons du pack par action
- [x] 3.4 Visée (ADS : animations du pack, zoom, souffle retenu), recul procédural (vue + bras), dispersion (hanche / visée / mouvement / saut), inspection ; tirs et inspection interrompus au changement de posture
- [x] 3.4b Objets ramassables par DataAsset (`UMEItemData` : munitions par calibre, ressources), inventaire (`UMEInventoryComponent`, max par objet), réserve de l'arme = munitions de son calibre dans l'inventaire (avance sur 7.1 / 7.2)
- [x] 3.4c Douilles éjectées : physique, vue FPS (partent de la fenêtre d'éjection telle qu'on la voit), 5 s au sol, pool de douilles recyclées, son d'impact prêt (à trouver)
- [x] 3.4d Réticule dynamique par DataAsset (`UMECrosshairData`, images du pack CrosshairFreePack) : s'écarte avec la dispersion, disparaît en visée ; défaut sur le controller, surcharge possible par arme
- [x] 3.4e Weapon sway (inertie de l'arme, par DataAsset) : retard sur la vue, mouvement / strafe, saut et réception, respiration idle ; bras + arme bougent d'un bloc autour de la poignée (nœud `ME Arms Offset`, aussi pour le recul), caméra immobile ; réduit en visée
- [x] 3.5 Cibles avec santé (`UMEHealthComponent`, réutilisable partout) + hit marker discret (images `T_HM_*`)
- [~] 3.6 Impacts : decals + particules par surface faits (Niagara Examples Pack, gratuit) ; **reste les sons et le sang** → voir *Assets à trouver*

**Fini quand :** tirer sur des cibles est satisfaisant.
💰 **Achat d'assets :** pack bras FPS + animations d'armes (l'achat le plus important du projet).

## Phase 4 — Survie
- [x] 4.1 Santé joueur (`UMEHealthComponent`), dégâts de chute, ressenti sans HUD (`UMEDamageFeedbackComponent` : flash + sursaut de vue au coup, vignette qui bat avec le cœur et couleurs délavées sous 50 %, grognements / cri de mort aléatoires), mort (vue qui s'écroule, fondu au noir) puis respawn ; commandes `MEDamage` / `MEHeal` / `MEKill`
- [~] 4.2 Medkit sur Q (`UMEMedkitComponent`, objet d'inventaire `DA_Item_Medkit`) : arme rangée, injection, santé rendue sur 1,5 s ; geste « arme rangée » réutilisable (`PutAwayFor`, Holster / Draw du pack) ; **reste l'animation medkit et le son** → voir *Assets à trouver*
  - Panneau de debug (`AMEHUD`) : santé, inventaire, arme, mouvement ; **Show Debug** dans `BP_MEGameMode`, commande `MEDebug`
- [x] 4.3 Zones toxiques (`AMEToxicZone` / `BP_ToxicZone`, densité réglable) : toxicité qui monte dans le gaz et retombe à l'air libre, dégâts au-dessus d'un seuil (sans grognement ni à-coup), toux de plus en plus fréquente, vision verdâtre / vignette / aberration ; la plus dense des zones superposées compte
- [~] 4.4 Masque à gaz sur G (`UMEGasMaskComponent`) : tap = mettre / retirer (arme rangée, protège une fois mis), maintenu = changer de filtre (objet `DA_Item_Filter`, 3 min par filtre, usé seulement masque porté) ; respiration dans le masque calme / haletante selon l'essoufflement et en fin de filtre ; **reste l'animation et les sons du masque** → voir *Assets à trouver*
- [~] 4.5 Visière (`UMEVisorComponent` + post-process `M_PP_Visor`) : contour du masque, buée qui suit l'essoufflement et la respiration, essuyage sur X, fissures aux coups (3e = visière brisée, le masque ne protège plus) ; **base en place, rendu du post-process à refaire** (textures → voir *Assets à trouver*)
- [-] 4.6 Montre au poignet → reportée : à repenser plus tard (éviter le copier-coller de Metro) ; le chrono du filtre passe par la respiration en attendant

**Fini quand :** traverser une zone toxique est stressant et lisible sans HUD.

## Phase 5 — IA humaine
Ennemis = occupants de la station (groupes de 2 à 4). **StateTree** (tâches en C++, l'asset ne fait que les relier).
Comportement décidé : combat **tactique crédible**, **détection progressive** (jauge selon distance / posture / mouvement / lumière),
un bruit attire l'ennemi vers l'endroit sans révéler le joueur. États : calme (patrouille, routine) → suspicion (vérifie) →
alerte (prévient, fouille, alarme) → combat → recherche (redescend par l'alerte, jamais directement au calme).

- [x] 5.1 `AMEAICharacter` (santé, corps MetaHuman, arme du monde) + `AMEAIController` (StateTree) ; chemins de patrouille `AMEPatrolPath` (spline, réglages par point : attente, regard, routine ; boucle / aller-retour ; poste fixe = 1 point) ; StateTree `ST_Enemy_Human` généré par `UMEEditorAILibrary` ; **provisoire** : corps d'animation MetaHuman invisible + Manny visible qui recopie la pose (`ABP_Enemy_Manny`, retarget) → un seul corps Manny dès qu'on a des animations ennemies
- [x] 5.2 Perception (`AMEAIController`) : vue en cône + jauge de détection (distance, côté du cône, accroupi, immobile, sprint ; tient 2 s puis redescend), ouïe via `MENoise` (pas selon l'allure, réceptions, tirs, douilles, portes ; force réglable par source) ; debug : jauge au-dessus des ennemis + « ? bruit »
- [ ] 5.3 États calme → suspicion → alerte → recherche ; remarque les changements (lumière éteinte, porte ouverte)
- [ ] 5.4 Combat : couverts (EQS), rafales, tickets d'attaque (1-2 tireurs à la fois), contournement, débusquer (suppression, grenade)
- [ ] 5.5 Groupe : alerte transmise à la voix (portée + délai), voix de combat (barks), appel au collègue qui ne répond pas, alarme de la station
- [ ] 5.6 Personnalité : profils par DataAsset (recrue / vétéran : courage, précision, agressivité, réaction), moral, fuite, reddition (épargner ou non), blessures (boite, appelle à l'aide)
- [ ] 5.7 Monde : routines de vie (fumer, feu, discuter, dormir), évite le gaz sans masque (tousse dedans), cadavre trouvé = alerte, lampes torches dans le noir
- [ ] 5.8 Mort, ragdoll (loot sur le corps → 7.2)

**Fini quand :** un groupe de 3 ennemis offre un combat crédible.

## Phase 6 — Lumière et infiltration
- [ ] 6.1 Calcul de visibilité du joueur (lumières proches + ombre)
- [ ] 6.2 Indicateur sur la montre (LED)
- [ ] 6.3 Lampe torche (batterie + recharge manuelle) et briquet
- [ ] 6.4 Lumières destructibles / éteignables
- [ ] 6.5 La perception IA utilise la visibilité du joueur
- [ ] 6.6 Takedown silencieux (assommer / tuer) par derrière

**Fini quand :** on peut nettoyer une zone sans être repéré, en jouant avec l'ombre.

## Phase 7 — Inventaire et crafting
- [ ] 7.1 Ressources (communes / techniques) + munitions + consommables
- [ ] 7.2 Loot dans le monde et sur les corps
- [ ] 7.3 Menu sac à dos (crafting filtres, medkits, munitions de base)
- [ ] 7.4 Nettoyage d'arme / encrassement (optionnel slice)

**Fini quand :** la boucle explorer → looter → crafter → survivre tourne.

## Phase 8 — Mutants
- [ ] 8.1 Mutant de mêlée (bondit, attaque, fuit la lumière ?)
- [ ] 8.2 Comportement de meute simple

**Fini quand :** une rencontre mutants dans le tunnel fait peur.

## Phase 9 — Construire la slice
- [ ] 9.1 Level design final : tunnel → zone toxique → station occupée → sortie
- [ ] 9.2 Sauvegarde / checkpoints
- [ ] 9.3 Menu principal, pause, options (sensibilité, graphismes, volume)
- [ ] 9.4 Audio : compléter les sons de pas (terre, neige, gravats… selon la map), ambiance, musique ; éclairage final
- [ ] 9.5 Optimisation (profiling Unreal Insights, LODs, culling)
- [ ] 9.6 Playtests + corrections

**Fini quand :** quelqu'un qui n'a jamais vu le projet le termine et en redemande.

---

## Assets à trouver
Liste tenue à jour à chaque étape : ce qui manque pour finir une mécanique déjà codée (le code attend juste l'asset).
On cherche d'abord du gratuit / pas cher (Fab, Epic, Freesound), licence vérifiée **avant** l'achat.

| Besoin | Pour | Où le brancher | Statut |
|---|---|---|---|
| Sons d'impact de balle : béton, métal, bois, chair | 3.6 | `DA_Impacts_Bullet` → Sound par surface | à trouver |
| Effet de sang (particules + decal) | 3.6 | `DA_Impacts_Bullet` → surface `PM_Flesh` | à trouver |
| Sons de douilles qui tombent au sol | 3.4c | `BP_ShellCasing_556` → son d'impact | à trouver |
| Son de douleur du joueur (grognement, plusieurs variantes) | 4.1 | `BP_MECharacter` → DamageFeedback → Hurt Sound | branché : `SC_Player_Hurt` (HumanVocalizations, voix Male A) |
| Battement de cœur **en boucle** | 4.1 | `BP_MECharacter` → DamageFeedback → Heartbeat Sound | à trouver |
| Son de mort du joueur | 4.1 | `BP_MECharacter` → DamageFeedback → Death Sound | branché : `SC_Player_Death` (HumanVocalizations, voix Male A) |
| Animation medkit des bras FPS (main gauche, arme en main droite) ; en attendant : arme rangée (Holster) puis ressortie | 4.2 | `UMEMedkitComponent` + un clip par arme dans `DA_Weapon_*` | à trouver |
| Son d'injection / de seringue | 4.2 | `BP_MECharacter` → Medkit → Inject Sound | à trouver |
| Mesh de medkit (objet ramassable) | 4.2 | `DA_Item_Medkit` → World Mesh + un `BP_Pickup_Medkit` | à trouver |
| Visuel du gaz : brouillard / particules (Local Fog Volume ou Niagara) | 4.3 | `BP_ToxicZone` → composant ajouté dans le BP | à trouver / à faire |
| Toux, respiration sous masque (marche / course) | 4.4 | `BP_MECharacter` → GasMask → Calm / Hard Breath Sound | branché : `SC_Mask_Breath_Calm` / `SC_Mask_Breath_Hard` (HumanVocalizations, Male A) |
| Animation des bras FPS : mettre / retirer le masque, changer le filtre (main gauche) ; en attendant : arme rangée | 4.4 | `UMEGasMaskComponent` | à trouver |
| Sons du masque : mettre, retirer, visser un filtre | 4.4 | `BP_MECharacter` → GasMask → Put On / Take Off / Change Filter Sound | à trouver |
| Mesh de filtre (objet ramassable) | 4.4 | `DA_Item_Filter` → World Mesh (cylindre en attendant) | à trouver |
| Textures de visière : fissures, buée, rayures (remplacent le procédural) | 4.5 | `M_PP_Visor` | à trouver (procédural en attendant) |
| Sons de visière : fissure, visière brisée, essuyage | 4.5 | `BP_MECharacter` → Visor → Crack / Break / Wipe Sound | à trouver |
| Animation des bras FPS : essuyer la visière (main gauche) ; en attendant : arme rangée | 4.5 | `UMEVisorComponent` | à trouver |
| Animations ennemi **sur Manny** : locomotion fusil, visée, tir, recharge, couvert, réactions aux coups, morts (piste gratuite : Lyra, licence à vérifier) ; permettront un seul corps | 5.1 → 5.4 | AnimBP ennemi sur `BP_MEAICharacter` | à trouver |
| Grenade : mesh, son, explosion | 5.4 | grenade ennemie | à trouver |
| Voix d'ennemis avec des mots (barks : « recharge ! », « il est là ! »…) ; HumanVocalizations n'a que grognements / souffles | 5.5 | profil d'ennemi | à trouver |
| Animations de reddition et de boiterie | 5.6 | AnimBP ennemi | à trouver |
| Animations de routines : fumer, assis, dormir, se chauffer | 5.7 | `AMEPatrolPath` → Routine | à trouver |
| Mesh de lampe torche (ennemis, puis joueur en 6.3) | 5.7 | `BP_MEAICharacter` | à trouver |

---

## Hors slice (plus tard, si financement)
Customisation d'armes sur établi, cycle jour/nuit, niveaux ouverts, véhicule, dialogues / narration, PNJ alliés, plus d'armes et de mutants.
