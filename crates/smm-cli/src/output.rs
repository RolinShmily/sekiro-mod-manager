use comfy_table::modifiers::UTF8_ROUND_CORNERS;
use comfy_table::presets::UTF8_FULL;
use comfy_table::{Cell, ContentArrangement, Table};

/// Builds a bordered table with the given column headers, standard look.
pub fn new_table(headers: &[&str]) -> Table {
    let mut table = Table::new();
    table
        .load_preset(UTF8_FULL)
        .apply_modifier(UTF8_ROUND_CORNERS)
        .set_content_arrangement(ContentArrangement::Dynamic)
        .set_header(headers.iter().map(Cell::new));
    table
}

/// Builds a two-column "Field / Value" table.
pub fn kv_table() -> Table {
    new_table(&["Field", "Value"])
}

/// Formats raw bytes into a human-readable representation (e.g. "24.50 MB").
pub fn format_bytes(bytes: u64) -> String {
    const KB: u64 = 1024;
    const MB: u64 = KB * 1024;
    const GB: u64 = MB * 1024;

    if bytes >= GB {
        format!("{:.2} GB ({} bytes)", bytes as f64 / GB as f64, bytes)
    } else if bytes >= MB {
        format!("{:.2} MB ({} bytes)", bytes as f64 / MB as f64, bytes)
    } else if bytes >= KB {
        format!("{:.2} KB ({} bytes)", bytes as f64 / KB as f64, bytes)
    } else {
        format!("{} B", bytes)
    }
}

/// Human-friendly enabled/disabled status cell.
pub fn status_cell(enabled: bool) -> Cell {
    if enabled {
        Cell::new("Enabled")
            .fg(comfy_table::Color::Green)
            .add_attribute(comfy_table::Attribute::Bold)
    } else {
        Cell::new("Disabled").fg(comfy_table::Color::DarkGrey)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_format_bytes_utility() {
        assert_eq!(format_bytes(500), "500 B");
        assert!(format_bytes(2048).contains("2.00 KB"));
        assert!(format_bytes(1024 * 1024 * 5).contains("5.00 MB"));
        assert!(format_bytes(1024 * 1024 * 1024 * 2).contains("2.00 GB"));
    }

    #[test]
    fn test_table_factories() {
        let mut t = kv_table();
        t.add_row(vec![Cell::new("K"), Cell::new("V")]);
        assert_eq!(t.row_count(), 1);
        assert!(new_table(&["A", "B", "C"]).header().is_some());
    }
}
