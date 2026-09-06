"""Protect real format strings and structured translation markup against damage."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('catalogs', Path(__file__).resolve().parents[1]/'scripts/build-localizations.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class TranslationContractTests(unittest.TestCase):
    def test_arabic_preserves_dynamic_display_name(self):
        self.assertEqual([], module.validate_catalog({'error':'Could not apply color to %@.'}, {'error':'تعذّر تطبيق اللون على %@.'}, 'ar'))
    def test_missing_placeholder_is_rejected(self):
        self.assertTrue(module.validate_catalog({'count':'Restoring in %1 seconds.'}, {'count':'Restoring soon.'}, 'bad'))
    def test_duplicate_placeholder_is_rejected(self):
        self.assertTrue(module.validate_catalog({'count':'Restoring in %1 seconds.'}, {'count':'%1 %1'}, 'bad'))
    def test_link_or_attribute_mutation_is_rejected(self):
        source={'download':'<a href="https://example.org/app">Download</a>'}
        self.assertTrue(module.validate_catalog(source, {'download':'<a href="https://example.org/other">Télécharger</a>'}, 'bad'))
        self.assertEqual([],module.validate_catalog(source, {'download':'<a href="https://example.org/app">Télécharger</a>'}, 'fr'))
    def test_key_loss_does_not_silently_fall_back(self):
        self.assertTrue(module.validate_catalog({'a':'Quit','b':'Restore'}, {'a':'Quitter'}, 'bad'))
    def test_duplicate_json_keys_are_rejected(self):
        with self.assertRaises(ValueError): module.unique_object([('a','x'),('a','y')])
    def test_literal_percentage_is_not_a_placeholder(self):
        self.assertEqual(module.placeholders('13% less contrast'), {})
    def test_changed_recovery_delay_or_version_is_rejected(self):
        self.assertTrue(module.validate_catalog({'a':'Restore after 15 seconds on Windows 11.'}, {'a':'Restore after 50 seconds on Windows 10.'}, 'bad'))
    def test_numeric_localized_month_and_spelled_count_are_allowed(self):
        self.assertTrue(module.numeric_values_preserved('SEPTEMBER 2, 2026', '2026年9月2日'))
        self.assertTrue(module.numeric_values_preserved('Save five curves.', '5つ保存'))
    def test_invalid_unicode_is_rejected(self):
        self.assertTrue(module.validate_catalog({'a':'Quit'}, {'a':'\ud800'}, 'bad'))

if __name__ == '__main__': unittest.main()
