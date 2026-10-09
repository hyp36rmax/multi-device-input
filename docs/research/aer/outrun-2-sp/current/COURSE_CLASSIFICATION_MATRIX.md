# Original Course Collision Classification Matrix

## Scope and source

This appendix records the 46 validated original collision resources used by the AER course study: 30 main-course (`coli_cs_*`) assets and 16 branch (`coli_bk_*`) assets.

The source archive is:

```text
data.zip
SHA-256: 6d0d6f98ad3ef03b74727a7aaa5e48917b570b0b82691a93ea5582eaf4502526
```

Each resource identifies as `SEGA-AM2 OUTRUN2` and uses the recovered `COLI0105` layout. Polygon counts and `ordinal:frequency` distributions below were recalculated directly from the one-byte classification table at the resource's `+0x18` offset. This confirms ordinal ownership and frequency; it does not assign a visual material name. [AER-EV-COLI-001](EVIDENCE_REGISTER.md#aer-ev-coli-001--coli0105-format) [AER-EV-COLI-004](EVIDENCE_REGISTER.md#aer-ev-coli-004--cross-course-validation)

## Native mask legend

| Code | Native mask | Relationship |
|---|---:|---|
| `P10C` | `0x00F03302` | Pattern 10 current-classification family |
| `P10P` | `0x0200801C` | Pattern 10 previous-classification family |
| `P12/13` | `0x00008014` | Patterns 12/13 front-contact family: ordinals 2, 4, 15 |
| `P14/15` | `0x02000008` | Patterns 14/15 front-contact family: ordinals 3, 25 |
| `ATTN` | `0xFF0FC07D` | Continuous-request per-front-contact attenuation family |

An ordinal appearing in a mask makes a polygon eligible for that part of the native decision tree. It does **not** guarantee that a pattern executes. Current and previous classifications, front-tire agreement, other vehicle state, countdown state, dirty state, and selection priority also participate. [AER-EV-MAG-002](EVIDENCE_REGISTER.md#aer-ev-mag-002--contact-attenuation) [AER-EV-PTN-003](EVIDENCE_REGISTER.md#aer-ev-ptn-003--pattern-10-transition) [AER-EV-PTN-004](EVIDENCE_REGISTER.md#aer-ev-ptn-004--patterns-1215)

The `Mask intersections` column lists only ordinals present in that resource. `Potential pattern eligibility` is structural, not proof of a runtime selection.

## Main-course resources

| Original asset | Polygons | Classification ordinal frequencies | Mask intersections | Potential pattern eligibility | Geometry / visual relationship | Confidence and uncertainty |
|---|---:|---|---|---|---|---|
| `coli_cs_alas_bin.gz` | 4,937 | 5:1242; 11:1242; 23:1439; 25:1014 | P10C:23; P10P:25; P14/15:25; ATTN:5,25 | P10 transition; P14/15 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_alpi_bin.gz` | 6,764 | 1:1638; 2:1885; 4:495; 5:61; 9:122; 11:1312; 16:602; 17:133; 18:302; 19:214 | P10C:1,9; P10P:2,4; P12/13:2,4; ATTN:2,4,5,16,17,18,19 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_amaz_bin.gz` | 5,072 | 1:1406; 2:1170; 5:1248; 11:1248 | P10C:1; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_beac_bin.gz` | 5,597 | 1:1113; 2:301; 3:1246; 5:984; 7:216; 10:139; 11:1370; 14:225; 16:3 | P10C:1; P10P:2,3; P12/13:2; P14/15:3; ATTN:2,3,5,14,16 | P10 transition; P12–15 families | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_cape_bin.gz` | 4,417 | 1:1413; 2:2; 3:678; 5:1128; 11:1162; 18:34 | P10C:1; P10P:2,3; P12/13:2; P14/15:3; ATTN:2,3,5,18 | P10 transition; P12–15 families | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_cast_bin.gz` | 6,643 | 1:1515; 2:2564; 11:1282; 17:1282 | P10C:1; P10P:2; P12/13:2; ATTN:2,17 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_clou_bin.gz` | 6,770 | 1:1378; 2:2696; 11:1348; 16:1348 | P10C:1; P10P:2; P12/13:2; ATTN:2,16 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_dese_bin.gz` | 5,336 | 1:1472; 3:1288; 8:6; 11:1282; 16:724; 18:564 | P10C:1,8; P10P:3; P14/15:3; ATTN:3,16,18 | P10 transition; P14/15 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_east_bin.gz` | 6,477 | 1:1511; 2:2177; 4:129; 5:1330; 11:1330 | P10C:1; P10P:2,4; P12/13:2,4; ATTN:2,4,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_flor_bin.gz` | 5,657 | 1:1769; 5:1626; 11:2262 | P10C:1; ATTN:5 | No complete P10 current→previous pair in this asset; attenuation eligible | COLI geometry verified; visual owner not traced | Counts CONFIRMED; runtime transition UNKNOWN |
| `coli_cs_fore_bin.gz` | 4,087 | 1:1453; 2:190; 11:1222; 19:1222 | P10C:1; P10P:2; P12/13:2; ATTN:2,19 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_ghos_bin.gz` | 5,299 | 1:1489; 2:1270; 8:1270; 16:1270 | P10C:1,8; P10P:2; P12/13:2; ATTN:2,16 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_gran_bin.gz` | 5,233 | 1:1332; 4:1301; 11:1300; 16:1300 | P10C:1; P10P:4; P12/13:4; ATTN:4,16 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_impe_bin.gz` | 4,611 | 1:1448; 11:1855; 16:204; 19:1104 | P10C:1; ATTN:16,19 | No complete P10 current→previous pair in this asset; attenuation eligible | COLI geometry verified; visual owner not traced | Counts CONFIRMED; runtime transition UNKNOWN |
| `coli_cs_indu_bin.gz` | 3,571 | 1:635; 5:51; 11:1666; 19:656; 24:563 | P10C:1; ATTN:5,19,24 | No complete P10 current→previous pair in this asset; attenuation eligible | COLI geometry verified; visual owner not traced | Counts CONFIRMED; runtime transition UNKNOWN |
| `coli_cs_lake_bin.gz` | 6,211 | 1:1263; 2:1608; 8:2; 9:470; 11:1370; 12:82; 16:804; 17:80; 19:470; 20:62 | P10C:1,8,9,12,20; P10P:2; P12/13:2; ATTN:2,16,17,19 | P10 transition, including ordinal 20 eligibility; P12/13 family | Ordinal 20 is one localized `1→20→1` run; visual owner not traced | Counts/sequence CONFIRMED; material UNKNOWN |
| `coli_cs_lasv_bin.gz` | 4,927 | 1:181; 5:1470; 7:1985; 11:642; 22:649 | P10C:1,22; ATTN:5 | No complete P10 current→previous pair in this asset; attenuation eligible | COLI geometry verified; visual owner not traced | Counts CONFIRMED; runtime transition UNKNOWN |
| `coli_cs_mach_bin.gz` | 6,698 | 1:1325; 2:2701; 11:1336; 16:1336 | P10C:1; P10P:2; P12/13:2; ATTN:2,16 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_maya_bin.gz` | 6,912 | 1:1607; 2:1107; 4:670; 12:2174; 16:1110; 17:204; 19:40 | P10C:1,12; P10P:2,4; P12/13:2,4; ATTN:2,4,16,17,19 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_metr_bin.gz` | 4,660 | 1:1378; 5:94; 10:1334; 11:627; 19:1227 | P10C:1; ATTN:5,19 | No complete P10 current→previous pair in this asset; attenuation eligible | COLI geometry verified; visual owner not traced | Counts CONFIRMED; runtime transition UNKNOWN |
| `coli_cs_newy_bin.gz` | 3,972 | 1:855; 5:1210; 10:697; 11:1210 | P10C:1; ATTN:5 | No complete P10 current→previous pair in this asset; attenuation eligible | COLI geometry verified; visual owner not traced | Counts CONFIRMED; runtime transition UNKNOWN |
| `coli_cs_niag_bin.gz` | 5,384 | 1:1388; 2:1084; 5:433; 10:4; 11:1576; 16:899 | P10C:1; P10P:2; P12/13:2; ATTN:2,5,16 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_palm_bin.gz` | 5,272 | 1:1283; 2:265; 3:644; 5:848; 7:344; 10:136; 11:1402; 14:350 | P10C:1; P10P:2,3; P12/13:2; P14/15:3; ATTN:2,3,5,14 | P10 transition; P12–15 families | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_prin_bin.gz` | 6,812 | 1:1452; 2:1007; 4:1563; 11:1398; 16:1288; 17:52; 20:52 | P10C:1,20; P10P:2,4; P12/13:2,4; ATTN:2,4,16,17 | P10 transition, including ordinal 20 eligibility; P12/13 family | Ordinal 20 is one localized `1→20→1` run; visual owner not traced | Counts/sequence CONFIRMED; material UNKNOWN |
| `coli_cs_ruin_bin.gz` | 6,611 | 1:1551; 3:2532; 11:1262; 16:660; 17:156; 18:450 | P10C:1; P10P:3; P14/15:3; ATTN:3,16,17,18 | P10 transition; P14/15 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_sanf_bin.gz` | 4,322 | 1:1134; 2:300; 5:960; 10:160; 11:1364; 19:404 | P10C:1; P10P:2; P12/13:2; ATTN:2,5,19 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_sequ_bin.gz` | 6,600 | 1:1424; 4:1294; 11:2588; 16:1294 | P10C:1; P10P:4; P12/13:4; ATTN:4,16 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_snow_bin.gz` | 5,491 | 1:174; 5:144; 10:144; 11:1218; 16:974; 17:244; 23:1375; 25:1218 | P10C:1,23; P10P:25; P14/15:25; ATTN:5,16,17,25 | P10 transition; P14/15 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_cs_tuli_bin.gz` | 6,208 | 1:1333; 2:1016; 3:1079; 11:1457; 16:1048; 19:236; 20:39 | P10C:1,20; P10P:2,3; P12/13:2; P14/15:3; ATTN:2,3,16,19 | P10 transition, including ordinal 20 eligibility; P12–15 families | Ordinal 20 polygons 124–162; exact visual registration; `re_CS_TULI_05_H_BLIDGE` bridge roadway | Counts/geometry/material CONFIRMED; cobblestone relationship STRONGLY SUPPORTED |
| `coli_cs_yose_bin.gz` | 5,384 | 1:1466; 2:1218; 11:1372; 16:1254; 17:74 | P10C:1; P10P:2; P12/13:2; ATTN:2,16,17 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |

## Branch resources

Fourteen branch resources share the same 610-polygon classification distribution. They remain listed separately because each is an original asset identity. `coli_bk_palm_bin.gz` and `coli_bk_ruin_bin.gz` differ.

| Original asset | Polygons | Classification ordinal frequencies | Mask intersections | Potential pattern eligibility | Geometry / visual relationship | Confidence and uncertainty |
|---|---:|---|---|---|---|---|
| `coli_bk_cape_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_east_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_enda_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_endb_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_endc_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_endd_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_ende_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_flor_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_impe_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_maya_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_metr_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_newy_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_palm_bin.gz` | 3,826 | 1:476; 2:1060; 5:1106; 8:2; 10:110; 13:1072 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_prin_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_ruin_bin.gz` | 567 | 1:81; 2:162; 5:162; 13:162 | P10C:1,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | COLI geometry verified; visual owner not traced | Counts CONFIRMED; material UNKNOWN |
| `coli_bk_tuli_bin.gz` | 610 | 1:90; 2:172; 5:176; 8:4; 13:168 | P10C:1,8,13; P10P:2; P12/13:2; ATTN:2,5 | P10 transition; P12/13 family | Separate Tulip branch asset; no ordinal 20 | Counts CONFIRMED; material UNKNOWN |

## Tulip Garden detail

Tulip's main and branch resources are distinct:

```text
coli_cs_tuli_bin.gz  main course  6,208 polygons
coli_bk_tuli_bin.gz  branch         610 polygons
```

Only the main course contains ordinal 20: 39 polygons, indices 124–162, forming one localized `1 → 20 → 1` sequence. Original visual and collision coordinates align exactly. The dominant visual owner and material lineage are:

```text
re_CS_TULI_05_H_BLIDGE
    draw batches 22/23
    → material records 13/14
    → texture IDs 0x46/0x47
    → STG_5A_H_BLIDGE_LORD_03/_04

batch 24 / material 15
    → STG_5A_H_BLIDGE_KAGE

six ROAD-object marking matches
    → O2S_XX_K_MICHI_HAKUSEN3

following ordinal-1 roadway
    → O2S_XX_K_MICHI_ASFA5A
```

Ordinal 20 intersects Pattern 10's current-family mask. That is eligibility, not proof that Pattern 10 fires on every polygon or that the board generates continuous vibration. The relationship to Tulip Garden's recognizable cobblestone section is **STRONGLY SUPPORTED**; the original names do not explicitly identify cobblestone. [AER-EV-VIS-002](EVIDENCE_REGISTER.md#aer-ev-vis-002--tulip-ordinal-20-visual-lineage) [AER-EV-VIS-003](EVIDENCE_REGISTER.md#aer-ev-vis-003--cobblestone-interpretation)

## Reproduction notes

To reproduce the matrix:

1. Verify the `data.zip` SHA-256 above.
2. Select the 46 `data/coli_cs_*_bin.gz` and `data/coli_bk_*_bin.gz` resources.
3. Remove the archive member's `or2 <size> ` wrapper and decompress its embedded gzip stream.
4. Verify `COLI0105` and the `SEGA-AM2 OUTRUN2` identity.
5. Read the little-endian polygon count at `+0x08`.
6. Read the classification-table offset at `+0x18`.
7. Count exactly one unsigned-byte ordinal per polygon.

Visual/material ownership has only been established for the traced Tulip region. All other visual names remain unknown until their own collision-to-visual lineage is reproduced.
