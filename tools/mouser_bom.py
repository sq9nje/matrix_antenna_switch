#!/usr/bin/env python3
"""Wycena BOM w Mouser Search API (tylko `requests`).

Klucz API czytany WYŁĄCZNIE ze zmiennej środowiskowej MOUSER_API_KEY:
    export MOUSER_API_KEY='...'
    python3 tools/mouser_bom.py [--out katalog]

Wyniki:
    mouser_wycena.csv   - pozycje + zamienniki (wiersze typ=ZAMIENNIK)
    mouser_bom_tool.csv - import do Mouser BOM Tool (MPN + ilość)
"""
import argparse
import csv
import os
import re
import sys
import time
from collections import OrderedDict, deque

import requests

API = "https://api.mouser.com/api/v1/search"
MAX_PER_MIN = 28          # limit Mousera ~30/min, zostawiamy margines
BATCH = 10                # partnumber search przyjmuje do 10 numerów (separator |)

# (płytka, opis, ilość, MPN lub None, słowa kluczowe, regex footprintu)
ITEMS = [
    # --- RELAY ---
    ("RELAY", "Przekaźnik FTR-K1CK012W", 18, "FTR-K1CK012W", "FTR-K1CK012W relay", None),
    ("RELAY", "GDT SL1002A600SM", 8, "SL1002A600SM", "gas discharge tube 600V SMD", None),
    ("RELAY", "Tranzystor BC817-40", 12, "BC817-40", "BC817-40 SOT-23", r"SOT-?23"),
    ("RELAY", "Dioda 1N4148WS", 18, "1N4148WS", "1N4148WS SOD-323", r"SOD-?323"),
    ("RELAY", "Dioda ER1B", 1, "ER1B", "ER1B SMA", r"SMA|DO-214AC"),
    ("RELAY", "TVS 24V 0603 (TESDU24V)", 13, "TESDU24V", "TVS ESD diode 24V 0603", r"0603"),
    ("RELAY", "Bezpiecznik PTC 1206 0ZCJ0025AF2E", 1, "0ZCJ0025AF2E", "PTC resettable fuse 1206", r"1206"),
    ("RELAY", "Rezystor 100R 2512", 12, "RC2512FK-07100RL", "resistor 100 ohm 2512 1%", r"2512"),
    ("RELAY", "Rezystor 4.7k 0805", 13, "RC0805FR-074K7L", "resistor 4.7k 0805 1%", r"0805"),
    ("RELAY", "Rezystor 1k 0805", 12, "RC0805FR-071KL", "resistor 1k 0805 1%", r"0805"),
    ("RELAY", "Kondensator 1n 0805", 12, "CC0805KRX7R9BB102", "capacitor 1nF 0805 X7R", r"0805"),
    ("RELAY", "Kondensator 100n 0805", 1, "CC0805KRX7R9BB104", "capacitor 100nF 0805 X7R", r"0805"),
    ("RELAY", "Kondensator elektrolit. 470u radial D8 P3.5", 1, None,
     "aluminum electrolytic capacitor 470uF radial 8mm 3.5mm", r"470\s*[uµ]F"),
    ("RELAY", "LED 0805", 13, None, "LED 0805 standard", r"0805"),
    ("RELAY", "Dławik osiowy DIN0411 12.7mm (wartość nieznana!)", 2, None,
     "axial inductor 12.7mm", None),
    ("RELAY", "Gniazdo UHF SO-239 square-flange (UC1)", 8, None,
     "UHF SO-239 jack square flange", r"UHF|SO-?239"),
    ("RELAY", "Złącze IDC 2x7", 1, None, "IDC header 2x7 2.54mm shrouded", r"14|2\s*x\s*7"),
    ("RELAY", "Listwa zaciskowa 2-pin 5.08", 1, None, "terminal block 2 position 5.08mm", r"5\.08"),
    # --- INTERFACE ---
    ("INTERFACE", "MAX13487EESA+", 1, "MAX13487EESA+", "MAX13487EESA", None),
    ("INTERFACE", "Stabilizator LM7805 TO-220", 1, "L7805CV", "LM7805 TO-220", r"TO-?220"),
    ("INTERFACE", "TVS 24V 0603 (TESDU24V)", 4, "TESDU24V", "TVS ESD diode 24V 0603", r"0603"),
    ("INTERFACE", "Dioda ER1B", 1, "ER1B", "ER1B SMA", r"SMA|DO-214AC"),
    ("INTERFACE", "Bezpiecznik PTC 1206 0ZCJ0025AF2E", 1, "0ZCJ0025AF2E", "PTC resettable fuse 1206", r"1206"),
    ("INTERFACE", "LED 0805", 4, None, "LED 0805 standard", r"0805"),
    ("INTERFACE", "Rezystor 2.2k 0805", 3, "RC0805FR-072K2L", "resistor 2.2k 0805 1%", r"0805"),
    ("INTERFACE", "Rezystor 1k 0805", 2, "RC0805FR-071KL", "resistor 1k 0805 1%", r"0805"),
    ("INTERFACE", "Rezystor 120R 0805", 1, "RC0805FR-07120RL", "resistor 120 ohm 0805 1%", r"0805"),
    ("INTERFACE", "Rezystor 10k 0805", 2, "RC0805FR-0710KL", "resistor 10k 0805 1%", r"0805"),
    ("INTERFACE", "Rezystor 470R 0805", 1, "RC0805FR-07470RL", "resistor 470 ohm 0805 1%", r"0805"),
    ("INTERFACE", "Kondensator 100n 0805", 4, "CC0805KRX7R9BB104", "capacitor 100nF 0805 X7R", r"0805"),
    ("INTERFACE", "Kondensator 22u 1206", 1, "CL31A226KAHNNNE", "capacitor 22uF 1206 X5R", r"1206"),
    ("INTERFACE", "Kondensator 100u radial D8 P3.8", 2, None,
     "aluminum electrolytic capacitor 100uF radial 8mm 3.5mm", r"100\s*[uµ]F"),
    ("INTERFACE", "Przycisk C&K RS282G05A3", 1, "RS282G05A3", "C&K RS282G05A3 tactile switch", None),
    ("INTERFACE", "Listwa zaciskowa 2-pin 5.08", 2, None, "terminal block 2 position 5.08mm", r"5\.08"),
    ("INTERFACE", "Złącze IDC 2x7", 1, None, "IDC header 2x7 2.54mm shrouded", r"14|2\s*x\s*7"),
    ("INTERFACE", "Goldpin 2x3 2.54", 1, None, "pin header 2x3 2.54mm", r"2\s*x\s*3|6"),
    ("INTERFACE", "Gniazdo goldpin 1x15 (ESP32 DevKit V1)", 2, None,
     "female header 1x15 2.54mm", r"15"),
]


class Limiter:
    """Przesuwane okno: maks. MAX_PER_MIN zapytań na 60 s."""

    def __init__(self, n):
        self.n, self.t = n, deque()

    def wait(self):
        now = time.monotonic()
        while self.t and now - self.t[0] >= 60:
            self.t.popleft()
        if len(self.t) >= self.n:
            time.sleep(60 - (now - self.t[0]) + 0.2)
        self.t.append(time.monotonic())


class Mouser:
    def __init__(self, key):
        self.key = key
        self.lim = Limiter(MAX_PER_MIN)
        self.s = requests.Session()
        self.calls = 0

    def _post(self, endpoint, body):
        for attempt in range(5):
            self.lim.wait()
            self.calls += 1
            try:
                # klucz tylko w query; nie logujemy URL ani wyjątków z URL
                r = self.s.post(f"{API}/{endpoint}", params={"apiKey": self.key},
                                json=body, timeout=30)
            except requests.RequestException as e:
                print(f"  ! błąd sieci ({type(e).__name__}), ponawiam", file=sys.stderr)
                time.sleep(2 ** attempt)
                continue
            if r.status_code == 429 or r.status_code >= 500:
                time.sleep(5 * (attempt + 1))
                continue
            if r.status_code != 200:
                raise SystemExit(f"Mouser HTTP {r.status_code} dla /{endpoint}")
            data = r.json()
            errs = data.get("Errors") or []
            if errs:
                msg = "; ".join(e.get("Message", "") for e in errs)
                if "limit" in msg.lower() or "too many" in msg.lower():
                    time.sleep(30)
                    continue
                print(f"  ! API: {msg}", file=sys.stderr)
            return (data.get("SearchResults") or {}).get("Parts") or []
        return []

    def by_part(self, mpns):
        return self._post("partnumber", {"SearchByPartRequest": {
            "mouserPartNumber": "|".join(mpns), "partSearchOptions": "None"}})

    def by_keyword(self, kw, records=40):
        return self._post("keyword", {"SearchByKeywordRequest": {
            "keyword": kw, "records": records, "startingRecord": 0,
            "searchOptions": "InStock"}})


def norm(s):
    return re.sub(r"[^A-Z0-9]", "", (s or "").upper())


def num(s):
    """'$1,234.50' / '1 234,50 €' -> float."""
    s = re.sub(r"[^\d.,]", "", s or "")
    if not s:
        return None
    if "," in s and "." in s:
        dec = "," if s.rfind(",") > s.rfind(".") else "."
        s = s.replace("," if dec == "." else ".", "").replace(dec, ".")
    elif "," in s:
        s = s.replace(",", ".")
    try:
        return float(s)
    except ValueError:
        return None


def stock(p):
    v = num(p.get("AvailabilityInStock") or "") if p.get("AvailabilityInStock") else None
    if v is None:
        m = re.match(r"\s*([\d.,\s]+)\s*In Stock", p.get("Availability") or "", re.I)
        v = num(m.group(1)) if m else 0
    return int(v or 0)


def order_qty(p, need):
    mn = int(p.get("Min") or 1)
    mult = int(p.get("Mult") or 1) or 1
    q = max(need, mn)
    return q + (-q) % mult


def unit_price(p, qty):
    """Cena jednostkowa z progu obowiązującego dla qty (największy próg <= qty)."""
    br = sorted(((int(b["Quantity"]), num(b.get("Price"))) for b in p.get("PriceBreaks") or []
                 if num(b.get("Price")) is not None))
    if not br:
        return None, ""
    cur = next((b.get("Currency", "") for b in p["PriceBreaks"]), "")
    price = br[0][1]
    for q, pr in br:
        if q <= qty:
            price = pr
    return price, cur


def evaluate(p, need):
    q = order_qty(p, need)
    price, cur = unit_price(p, q)
    return {"part": p, "qty": q, "price": price, "cur": cur, "stock": stock(p)}


def usable(e, need):
    return e["price"] is not None and e["stock"] >= e["qty"]


def pick_best(cands, need):
    ok = [e for e in cands if usable(e, need)]
    return min(ok, key=lambda e: e["price"] * e["qty"]) if ok else None


def keyword_candidates(api, kw, need, pkg, exclude=""):
    out = []
    for p in api.by_keyword(kw):
        if pkg and not re.search(pkg, f"{p.get('Description', '')} {p.get('ManufacturerPartNumber', '')}", re.I):
            continue
        if exclude and norm(p.get("ManufacturerPartNumber")) == norm(exclude):
            continue
        out.append(evaluate(p, need))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=".", help="katalog na pliki CSV")
    args = ap.parse_args()
    key = os.environ.get("MOUSER_API_KEY")
    if not key:
        raise SystemExit("Ustaw zmienną środowiskową MOUSER_API_KEY (klucza nie wpisuj w kod).")
    api = Mouser(key)

    # 1) zapytania wsadowe po numerach części
    mpns = list(OrderedDict.fromkeys(i[3] for i in ITEMS if i[3]))
    found = []
    for k in range(0, len(mpns), BATCH):
        print(f"partnumber: {mpns[k:k + BATCH]}")
        found += api.by_part(mpns[k:k + BATCH])

    rows, bom, totals = [], OrderedDict(), {}
    for board, desc, need, mpn, kw, pkg in ITEMS:
        print(f"[{board}] {desc} x{need}")
        best, alts, note = None, [], ""
        if mpn:
            n = norm(mpn)
            cands = [evaluate(p, need) for p in found
                     if norm(p.get("ManufacturerPartNumber")).startswith(n)
                     or norm(p.get("MouserPartNumber")) == n]
            best = pick_best(cands, need)
            if not best and cands:                      # jest, ale brak stanu/ceny
                best = min(cands, key=lambda e: -e["stock"])
                note = "stan niewystarczający" if best["stock"] < best["qty"] else "brak cennika"
        if not best:                                    # część generyczna lub nie znaleziono
            cands = keyword_candidates(api, kw, need, pkg)
            best = pick_best(cands, need)
            note = note or ("dobrana wg słów kluczowych - zweryfikuj parametry"
                            if best else "brak w Mouserze")
        if not best or not usable(best, need):
            # 2–3 zamienniki o tym samym footprincie
            ex = best["part"]["ManufacturerPartNumber"] if best else (mpn or "")
            pool = [e for e in keyword_candidates(api, kw, need, pkg, ex) if usable(e, need)]
            alts = sorted(pool, key=lambda e: e["price"] * e["qty"])[:3]
            if not pkg:
                note += "; zamienniki: footprint do ręcznej weryfikacji"

        def row(e, typ):
            p = e["part"]
            cost = e["price"] * e["qty"] if e["price"] is not None else ""
            return {"typ": typ, "plytka": board, "opis_wymagany": desc, "opis_mouser": p.get("Description", ""),
                    "MPN": p.get("ManufacturerPartNumber", ""), "numer_mousera": p.get("MouserPartNumber", ""),
                    "ilosc_wymagana": need, "ilosc_zamawiana": e["qty"],
                    "cena_jedn": e["price"] if e["price"] is not None else "", "waluta": e["cur"],
                    "koszt": round(cost, 4) if cost != "" else "", "stan_magazynu": e["stock"],
                    "min_zamowienie": p.get("Min", ""), "wielokrotnosc": p.get("Mult", ""),
                    "link": p.get("ProductDetailUrl", ""), "uwagi": note if typ == "POZYCJA" else ""}

        if best:
            rows.append(row(best, "POZYCJA" if usable(best, need) else "POZYCJA-PROBLEM"))
            if usable(best, need):
                m = best["part"]["ManufacturerPartNumber"]
                bom[m] = bom.get(m, 0) + best["qty"]
                totals[best["cur"]] = totals.get(best["cur"], 0) + best["price"] * best["qty"]
        else:
            rows.append({"typ": "BRAK", "plytka": board, "opis_wymagany": desc, "ilosc_wymagana": need,
                         "uwagi": note})
        rows += [row(a, "ZAMIENNIK") for a in alts]

    os.makedirs(args.out, exist_ok=True)
    f1 = os.path.join(args.out, "mouser_wycena.csv")
    cols = ["typ", "plytka", "opis_wymagany", "opis_mouser", "MPN", "numer_mousera", "ilosc_wymagana",
            "ilosc_zamawiana", "cena_jedn", "waluta", "koszt", "stan_magazynu", "min_zamowienie",
            "wielokrotnosc", "link", "uwagi"]
    with open(f1, "w", newline="", encoding="utf-8-sig") as f:
        w = csv.DictWriter(f, cols, delimiter=";")
        w.writeheader()
        w.writerows(rows)
    f2 = os.path.join(args.out, "mouser_bom_tool.csv")
    with open(f2, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Manufacturer Part Number", "Quantity"])
        w.writerows(bom.items())

    probl = [r for r in rows if r["typ"] in ("POZYCJA-PROBLEM", "BRAK")]
    print(f"\nZapytań API: {api.calls}; pozycji w BOM: {len(bom)}; problemowych: {len(probl)}")
    for cur, t in totals.items():
        print(f"ŁĄCZNY KOSZT (bez dostawy/VAT): {t:.2f} {cur}")
    for r in probl:
        print(f"  ! {r['plytka']}: {r['opis_wymagany']} - {r.get('uwagi', '')}")
    print(f"Zapisano: {f1}, {f2}")


if __name__ == "__main__":
    main()
