CREATE OR REPLACE FUNCTION add_uram_points_and_recalc(
    p_h3_index BIGINT,
    p_user_id BIGINT,
    p_points_delta INT,
    p_distance_delta DOUBLE PRECISION
)
RETURNS TABLE (
    is_captured BOOLEAN,
    new_owner_id BIGINT,
    prev_owner_id BIGINT,
    new_top_score INT
) AS $$
DECLARE
    v_prev_owner_id BIGINT;
    v_top_user_id BIGINT;
    v_top_score INT;
    v_is_captured BOOLEAN := FALSE;
BEGIN
    INSERT INTO hexagon_user_stats (h3_index, user_id, uram_points, total_distance_meters, visits_count, last_visited_at)
    VALUES (p_h3_index, p_user_id, p_points_delta, p_distance_delta, 1, NOW())
    ON CONFLICT (h3_index, user_id) DO UPDATE 
    SET uram_points = hexagon_user_stats.uram_points + EXCLUDED.uram_points,
        total_distance_meters = hexagon_user_stats.total_distance_meters + EXCLUDED.total_distance_meters,
        visits_count = hexagon_user_stats.visits_count + 1,
        last_visited_at = NOW();

    SELECT owner_user_id INTO v_prev_owner_id FROM hexagons WHERE h3_index = p_h3_index FOR UPDATE;

    SELECT user_id, uram_points INTO v_top_user_id, v_top_score
    FROM hexagon_user_stats
    WHERE h3_index = p_h3_index
    ORDER BY uram_points DESC LIMIT 1;

    IF v_prev_owner_id IS NULL OR v_prev_owner_id != v_top_user_id THEN
        v_is_captured := TRUE;
        
        INSERT INTO hexagons (h3_index, owner_user_id, top_score, captured_at)
        VALUES (p_h3_index, v_top_user_id, v_top_score, NOW())
        ON CONFLICT (h3_index) DO UPDATE
        SET owner_user_id = EXCLUDED.owner_user_id,
            top_score = EXCLUDED.top_score,
            captured_at = NOW();

        INSERT INTO hexagon_history (h3_index, previous_owner_id, new_owner_id, score_at_capture)
        VALUES (p_h3_index, v_prev_owner_id, v_top_user_id, v_top_score);

        IF v_prev_owner_id IS NOT NULL THEN
            UPDATE user_stats SET current_held_hexagons = GREATEST(0, current_held_hexagons - 1), updated_at = NOW() WHERE user_id = v_prev_owner_id;
        END IF;
        UPDATE user_stats SET current_held_hexagons = current_held_hexagons + 1, updated_at = NOW() WHERE user_id = v_top_user_id;
    ELSE
        UPDATE hexagons SET top_score = v_top_score WHERE h3_index = p_h3_index;
    END IF;

    RETURN QUERY SELECT v_is_captured, v_top_user_id, v_prev_owner_id, v_top_score;
END;
$$ LANGUAGE plpgsql;